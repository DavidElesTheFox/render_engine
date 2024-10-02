#include "ParallelDemoApplication.h"

#include <render_engine/containers/IndexSet.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/rendergraph/Node.h>
#include <render_engine/rendergraph/Topic.h>
#include <render_engine/synchronization/SyncObject.h>

#include <ApplicationContext.h>
#include <DeviceSelector.h>
#include <scene/Camera.h>
#include <WindowSetup.h>

#include <format>
#include <memory>
#include <thread>
#include <vector>

namespace
{
    namespace rg = RenderEngine::RenderGraph;

    static thread_local std::string g_thread_name = []
        {
            const std::thread::id id = std::this_thread::get_id();
            return std::format("Thread-{:}", *reinterpret_cast<const unsigned int*>(&id));
        }();


    class SwapChainImageSelector
    {
    public:
        SwapChainImageSelector(uint32_t image_count,
                               RenderEngine::LogicalDevice& logical_device,
                               RenderEngine::SwapChain& swap_chain)
            : _image_count(image_count)
            , _logical_device(logical_device)
            , _swap_chain(swap_chain)
        {}
        /**
          * @brief Determines the next available image index.
          *
          * This is tricky:
          * There are renders ongoing on the back buffers. Each back buffer has its own semaphore to trigger when the drawing are finished
          * Its gonna be the available_index. Its semaphore will be used to query the next back buffer.
          *
         */
        std::optional<rg::ExecutionContext::PoolIndex> getNextImage(rg::ExecutionContext& execution_context);

        void releasePoolIndex(const rg::ExecutionContext::PoolIndex& index)
        {
            auto lock = std::unique_lock(_occupied_indexes_mutex);
            _occupied_sync_object_indexes.erase(index.sync_object_index);
        }
    private:



        void markSyncIndexAsOccupied(uint32_t sync_index)
        {
            std::unique_lock lock{ _occupied_indexes_mutex };
            _occupied_sync_object_indexes.insert(sync_index);
        }
        RenderEngine::IndexSet<uint32_t> getOccupiedSyncIndexes() const
        {
            std::shared_lock lock(_occupied_indexes_mutex);
            return _occupied_sync_object_indexes;
        }

        uint32_t chooseSyncObjectIndex(const std::vector<const RenderEngine::SyncObject*>& sync_objects);

        std::optional<uint32_t> tryAcquireImage(rg::ExecutionContext& execution_context) const;

        mutable std::shared_mutex _occupied_indexes_mutex;
        RenderEngine::IndexSet<uint32_t> _occupied_sync_object_indexes; /**< On the CPU the draw calls are recording right now*/

        uint32_t _image_count{ 0 };
        RenderEngine::LogicalDevice& _logical_device;
        RenderEngine::SwapChain& _swap_chain;
        std::mutex _index_selection_mutex;

    };
    class ImageAcquireTask : public rg::CpuNode::ICpuTask
    {
    public:
        constexpr static auto image_available_semaphore_name = "image-available";
        ImageAcquireTask(RenderEngine::Window& window,
                         RenderEngine::LogicalDevice& logical_device,
                         uint32_t back_buffer_count,
                         std::string image_user_node_name)
            : _window(window)
            , _swap_chain_image_selector(back_buffer_count, logical_device, window.getSwapChain())
            , _image_user_node_name(std::move(image_user_node_name))
        {}
        void run(rg::ExecutionContext& execution_context) final
        {
            PROFILE_THREAD(g_thread_name.c_str());
            PROFILE_SCOPE();
            /*
            * When we are here we know that the process is finished. It means that the draw calls were issued on Host.
            * But in order to use the semaphore we need to wait until it has no more ongoing work on the GPU.
            * It means we need to wait until the execution is finished on the GPU:
            */
            auto sync_object_index = execution_context.getPoolIndex().sync_object_index;
            execution_context.getSyncFeedbackService().get(&execution_context.getSyncObject(sync_object_index), _image_user_node_name)->wait();
            // Issue is here with intel gpu :

            // Frame i is rendered by execution context j (j = i % parallel_frame_count)
            // Then sync_object index will be choosen by available sync objects (no host operation): k
            // When k is not equal to i then there is no guarantee that on the gpu there is no pending operation.
            std::optional<rg::ExecutionContext::PoolIndex> pool_index = _swap_chain_image_selector.getNextImage(execution_context);

            if (pool_index == std::nullopt)
            {
                _window.reinitSwapChain();
            }
            else
            {
                execution_context.setPoolIndex(*pool_index);
            }

        }

        void registerExecutionContext(rg::ExecutionContext& execution_context) override
        {
            PROFILE_SCOPE();
            rg::ExecutionContext::Events events
            {
                .on_pool_index_set = nullptr,
                .on_pool_index_clear = [this](const auto& index) { _swap_chain_image_selector.releasePoolIndex(index); }
            };
            execution_context.addEvents(std::move(events));
        };

        bool isActive() const final { return true; }
    private:

        RenderEngine::Window& _window;
        SwapChainImageSelector _swap_chain_image_selector;
        std::string _image_user_node_name;
    };


    std::optional<rg::ExecutionContext::PoolIndex> SwapChainImageSelector::getNextImage(rg::ExecutionContext& execution_context)
    {
        PROFILE_SCOPE();

        std::optional<uint32_t> image_index = tryAcquireImage(execution_context);

        if (image_index == std::nullopt)
        {
            return std::nullopt;
        }


        RenderEngine::RenderContext::context().getDebugger().print(RenderEngine::Debug::Topics::RenderGraphExecution{},
                                                                   "Image Acquire: Image index {:d} frame: {:d} (Using synchronization object: {:d}) [thread: {}]",
                                                                   *image_index,
                                                                   execution_context.getCurrentFrameNumber(),
                                                                   execution_context.getPoolIndex().sync_object_index,
                                                                   (std::stringstream{} << std::this_thread::get_id()).str());

        return rg::ExecutionContext::PoolIndex{ .render_target_index = *image_index, .sync_object_index = execution_context.getPoolIndex().sync_object_index };
    }
    uint32_t SwapChainImageSelector::chooseSyncObjectIndex(const std::vector<const RenderEngine::SyncObject*>& sync_objects)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_occupied_indexes_mutex);
        const uint32_t available_index = [&]
            {
                for (uint32_t i = 0; i < sync_objects.size(); ++i)
                {
                    if (_occupied_sync_object_indexes.contains(i) == false)
                    {
                        return i;
                    }
                }
                assert(false && "Couldn't find any available item in the pool");
                return 0u;
            }();
        _occupied_sync_object_indexes.insert(available_index);
        return available_index;
    }
    std::optional<uint32_t> SwapChainImageSelector::tryAcquireImage(rg::ExecutionContext& execution_context) const
    {
        PROFILE_SCOPE();
        using namespace std::chrono_literals;
        const auto timeout = 1ms;
        const auto wait_time = 1ms;

        RenderEngine::SwapChain::ImageAcquireResult acquire_result = {
            .image_index = 0,
            .result = VK_TIMEOUT
        };

        /*
        * Because forward progress is not guaranteed with this setup we need to specify a timeout.
        * I.e.: We are requesting 'too' much image from the swap chain.
        * See more details:
        * https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#VUID-vkAcquireNextImageKHR-surface-07783
        * https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#swapchain-acquire-forward-progress
        */
        while (acquire_result.result == VK_TIMEOUT)
        {
            PROFILE_SCOPE("SwapChainImageSelector::tryAcquireImage - probe");
            const uint64_t timeout_ns = timeout.count();
            acquire_result = _swap_chain.acquireNextImage(timeout_ns,
                                                          execution_context.getSyncObject(execution_context.getPoolIndex().sync_object_index)
                                                          .getPrimitives().getSemaphore(ImageAcquireTask::image_available_semaphore_name),
                                                          VK_NULL_HANDLE);
            if (acquire_result.result == VK_TIMEOUT)
            {
                PROFILE_SCOPE("SwapChainImageSelector::tryAcquireImage - sleep");

                // std::this_thread::sleep_for(wait_time);
            }
        }
        switch (acquire_result.result)
        {
            case VK_ERROR_OUT_OF_DATE_KHR:
            case VK_SUBOPTIMAL_KHR:
                // TODO replace it with an exception, while it is an exceptional case. Makes much simpler the interface
                return std::nullopt;
            case VK_SUCCESS:
                break;
            default:
                throw std::runtime_error("Failed to query swap chain image");
        }
        return acquire_result.image_index;
    }

}

void ParallelDemoApplication::createRenderEngine()
{
    auto& render_engine = static_cast<RenderEngine::ParallelRenderEngine&>(_window_setup->getRenderingWindow().getRenderEngine());

    RenderEngine::RenderGraph::RenderGraphBuilder builder = render_engine.createRenderGraphBuilder("StandardPipeline");
    const RenderEngine::RenderGraph::BinarySemaphore image_available_semaphore{ ImageAcquireTask::image_available_semaphore_name };
    builder.registerSemaphore(image_available_semaphore);

    ImageAcquireTask* image_acquire_task{ nullptr };
    {
        const auto base_render_target = _window_setup->getRenderingWindow().createRenderTarget();

        auto forward_renderer = std::make_unique<RenderEngine::ForwardRenderer>(render_engine,
                                                                                base_render_target.clone()
                                                                                .changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                                                                                false);
        _forward_renderer = forward_renderer.get();

        auto ui_renderer = std::make_unique<RenderEngine::UIRenderer>(_window_setup->getUiWindow(),
                                                                      base_render_target.clone()
                                                                      .changeInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                                                                      .changeLoadOperation(VK_ATTACHMENT_LOAD_OP_LOAD),
                                                                      3,
                                                                      false);
        _ui_renderer = ui_renderer.get();
        auto image_acquire_task_ptr = std::make_unique<ImageAcquireTask>(_window_setup->getUiWindow(),
                                                                         _window_setup->getUiWindow().getDevice().getLogicalDevice(),
                                                                         _window_setup->getBackbufferCount(),
                                                                         "ForwardRenderer");
        image_acquire_task = image_acquire_task_ptr.get();
        builder.addDeviceSynchronizeNode("SynchronizeRenderGpu", _window_setup->getRenderingWindow().getDevice());
        builder.addCpuNode("AcquireImage", std::move(image_acquire_task_ptr));
        builder.addRenderNode("ForwardRenderer", std::move(forward_renderer));
        builder.addRenderNode("ImguiRenderer", std::move(ui_renderer));
        builder.addPresentNode("Present", _window_setup->getUiWindow().getSwapChain());
        builder.addEmptyNode(RenderEngine::ParallelRenderEngine::kEndNodeName);
    }


    builder.addCpuSyncLink("AcquireImage", "ForwardRenderer")
        .signalOnGpu()
        .waitOnGpu(image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("AcquireImage", "SynchronizeRenderGpu");
    builder.addCpuSyncLink("SynchronizeRenderGpu", "ForwardRenderer");
    /* TODO Needs to implement a merge tasks into the data transfer scheduler.
    * The data is transferred per resources (each transfer does a submit). Thus, it will signal the semaphore n times when n buffer is synchronized.
    .signalOnGpu(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    .waitOnGpu(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    */
    builder.addCpuSyncLink("ForwardRenderer", "ImguiRenderer")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("ImguiRenderer", "Present")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    render_engine.setRenderGraph(builder.reset("none"));

    _ui_renderer->setOnGui(
        [&]
        {
            onGui();
        });
}

void ParallelDemoApplication::init()
{
    using namespace RenderEngine;

    initializeRenderers();
    createWindowSetup();

    createRenderEngine();

    createScene();

    DemoSceneBuilder demoSceneBuilder;
    _scene_resources = demoSceneBuilder.buildSceneOfQuads(_assets,
                                                          *_scene,
                                                          getRenderingWindow().getDevice(),
                                                          getRenderingWindow().getRenderEngine());

    ApplicationContext::instance().init(_scene.get(), getUiWindow().getWindowHandle());

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), _forward_renderer, nullptr);

    _render_manager->registerMeshesForRender();

    _asset_browser = std::make_unique<Ui::AssetBrowserUi>(_assets, *_scene);
}

void ParallelDemoApplication::createScene()
{
    Scene::Scene::SceneSetup scene_setup{
        .up = glm::vec3(0.0f, 1.0f, 0.0f),
        .forward = glm::vec3(0.0f, 0.0f, -1.0f)
    };

    _scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
}

void ParallelDemoApplication::run()
{
    auto& render_engine = static_cast<RenderEngine::ParallelRenderEngine&>(_window_setup->getRenderingWindow().getRenderEngine());

    while (getUiWindow().isClosed() == false)
    {
        ApplicationContext::instance().onFrameBegin();
        _window_setup->update();
        render_engine.render();
        ApplicationContext::instance().updateInputEvents();
        ApplicationContext::instance().onFrameEnd();
    }
}

ParallelDemoApplication::~ParallelDemoApplication()
{

    if (_window_setup != nullptr)
    {
        auto& render_engine = static_cast<RenderEngine::ParallelRenderEngine&>(_window_setup->getRenderingWindow().getRenderEngine());
        render_engine.waitIdle();
        // finish all rendering before destroying anything
        getRenderingWindow().getDevice().waitIdle();
        getUiWindow().getDevice().waitIdle();
    }
}
void ParallelDemoApplication::initializeRenderers()
{
    using namespace RenderEngine;

    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();

    DeviceSelector device_selector;
    RenderContext::InitializationInfo init_info{};
    init_info.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    init_info.app_info.apiVersion = VK_API_VERSION_1_3;
    init_info.app_info.applicationVersion = 0;
    init_info.app_info.engineVersion = 0;
    init_info.app_info.pApplicationName = "DemoApplication";
    init_info.app_info.pEngineName = "FoxEngine";
    init_info.enable_validation_layers = true;
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_NV_optimus" };
    init_info.renderer_factory = std::move(renderers);
    init_info.device_selector = [&](const DeviceLookup& lookup) ->VkPhysicalDevice { return device_selector.askForDevice(lookup); };
    init_info.queue_family_selector = [&](const DeviceLookup::DeviceInfo& info) { return device_selector.askForQueueFamilies(info); };
    RenderContext::initialize(std::move(init_info));

}

void ParallelDemoApplication::onGui()
{
    _asset_browser->onGui();
    if (_scene->getActiveCamera())
    {
        _scene->getActiveCamera()->onGui();
    }
    ApplicationContext::instance().onGui();
}

void ParallelDemoApplication::createWindowSetup()
{
    using namespace RenderEngine;
    constexpr bool use_offscreen_rendering = false;

    if constexpr (use_offscreen_rendering)
    {
        _window_setup = std::make_unique<OffScreenWindowSetup>(std::vector<uint32_t> {});
    }
    else
    {
        _window_setup = std::make_unique<SingleWindowSetup>(std::vector<uint32_t>{}, true);
    }

}
