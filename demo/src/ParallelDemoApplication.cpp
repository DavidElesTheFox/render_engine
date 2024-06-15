#include "ParallelDemoApplication.h"

#include <render_engine/containers/IndexSet.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/synchronization/SyncObject.h>

#include <ApplicationContext.h>
#include <DeviceSelector.h>
#include <scene/Camera.h>
#include <WindowSetup.h>

#include <memory>
#include <vector>

namespace
{
    namespace rg = RenderEngine::RenderGraph;

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
            auto lock = std::lock_guard(_access_mutex);
            _occupied_sync_object_indexes.erase(index.sync_object_index);
        }
    private:
        std::optional<uint32_t> findUnusedIndex() const
        {
            for (uint32_t i = 0; i < _image_count; ++i)
            {
                if (_used_sync_object_indexes.contains(i) == false)
                {
                    return i;
                }
            }
            return std::nullopt;
        }

        uint32_t findFinishedSyncObjectIndex(const std::vector<const RenderEngine::SyncObject*>& sync_objects) const;

        std::optional<uint32_t> tryAcquireImage(rg::ExecutionContext& execution_context,
                                                uint32_t available_index) const;

        std::mutex _access_mutex;
        RenderEngine::IndexSet<uint32_t> _occupied_sync_object_indexes; /**< On the CPU the draw calls are recording right now*/
        RenderEngine::IndexSet<uint32_t> _used_sync_object_indexes; /**< There were any draw call recorded ever */

        uint32_t _image_count{ 0 };
        RenderEngine::LogicalDevice& _logical_device;
        RenderEngine::SwapChain& _swap_chain;

    };

    class ImageAcquireTask : public rg::CpuNode::ICpuTask
    {
    public:
        constexpr static auto render_finished_semaphore_name = "render-finished";
        constexpr static auto image_available_semaphore_name = "image-available";
        ImageAcquireTask(RenderEngine::Window& window, RenderEngine::LogicalDevice& logical_device, uint32_t back_buffer_count)
            : _window(window)
            , _swap_chain_image_selector(back_buffer_count, logical_device, window.getSwapChain())
        {}
        void run(rg::ExecutionContext& execution_context) final
        {
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

        void register_execution_context(rg::ExecutionContext& execution_context) override
        {
            rg::ExecutionContext::Events events
            {
                .on_pool_index_set = nullptr,
                .on_pool_index_clear = [this](const auto& index) { _swap_chain_image_selector.releasePoolIndex(index); }
            };
            execution_context.add_events(std::move(events));
        };

        bool isActive() const final { return true; }
    private:

        RenderEngine::Window& _window;
        SwapChainImageSelector _swap_chain_image_selector;
    };

    /**
    * @brief Determines the next available image index.
    *
    * This is tricky:
    * There are renders ongoing on the back buffers. Each back buffer has its own semaphore to trigger when the drawing are finished
    * Its gonna be the available_index. Its semaphore will be used to query the next back buffer.
    *
    */

    inline std::optional<rg::ExecutionContext::PoolIndex> SwapChainImageSelector::getNextImage(rg::ExecutionContext& execution_context)
    {
        auto lock = std::lock_guard(_access_mutex);
        auto sync_object_holder = execution_context.getAllSyncObject();

        std::optional<uint32_t> sync_object_index = findUnusedIndex();
        if (sync_object_index == std::nullopt)
        {
            sync_object_index = findFinishedSyncObjectIndex(sync_object_holder.sync_objects);
        }

        std::optional<uint32_t> image_index = tryAcquireImage(execution_context,
                                                              *sync_object_index);

        if (image_index == std::nullopt)
        {
            return std::nullopt;
        }
        // Semaphore was used only when the index was used before, thus when it is a new insertion.
        const bool semaphore_was_used = _used_sync_object_indexes.insert(*sync_object_index) == false;

        _occupied_sync_object_indexes.insert(*image_index);

        RenderEngine::RenderContext::context().getDebugger().print("[WAT] Image Acquire: Image index {:d} (Using synchronization object: {:d}", *image_index, *sync_object_index);

        // !! Unlocking manually. Not fun of this but makes readable the code (i.e.: Dealing with image index, and semaphore_was_used)
        sync_object_holder.lock.unlock();

        if (semaphore_was_used)
        {
            execution_context.stepTimelineSemaphore(*sync_object_index, ImageAcquireTask::render_finished_semaphore_name);
        }
        return rg::ExecutionContext::PoolIndex{ .render_target_index = *image_index, .sync_object_index = *sync_object_index };
    }
    inline uint32_t SwapChainImageSelector::findFinishedSyncObjectIndex(const std::vector<const RenderEngine::SyncObject*>& sync_objects) const
    {
        auto black_list = _used_sync_object_indexes.negate(); // Do not count those where no draw cal were recorded.
        black_list.insert(_occupied_sync_object_indexes); // Do not count those where draw calls are recording right now.
        const RenderEngine::SyncObject* available_object = RenderEngine::SyncObject::SharedOperations::from(sync_objects)
            .waitAnyOfSemaphores(ImageAcquireTask::render_finished_semaphore_name, 1, black_list);
        const auto available_index = static_cast<uint32_t>(std::ranges::find(sync_objects, available_object) - sync_objects.begin());
        return available_index;
    }
    inline std::optional<uint32_t> SwapChainImageSelector::tryAcquireImage(rg::ExecutionContext& execution_context, uint32_t available_index) const
    {
        using namespace std::chrono_literals;
        const auto timeout = 30ms;

        VkResult call_result{ VK_TIMEOUT };
        uint32_t image_index = 0;

        /*
        * Because forward progress is not guaranteed with this setup we need to specify a timeout.
        * I.e.: We are requesting 'too' much image from the swap chain.
        * See more details:
        * https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#VUID-vkAcquireNextImageKHR-surface-07783
        * https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#swapchain-acquire-forward-progress
        */
        while (call_result == VK_TIMEOUT)
        {
            std::tie(call_result, image_index) =
                execution_context.getSyncObject(available_index).sync_object.acquireNextSwapChainImage(_logical_device,
                                                                                                       _swap_chain.getDetails().swap_chain,
                                                                                                       ImageAcquireTask::image_available_semaphore_name,
                                                                                                       timeout);
        }
        switch (call_result)
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
        return image_index;
    }
}

void ParallelDemoApplication::createRenderEngine()
{
    auto& render_engine = static_cast<RenderEngine::ParallelRenderEngine&>(_window_setup->getRenderingWindow().getRenderEngine());

    RenderEngine::RenderGraph::RenderGraphBuilder builder = render_engine.createRenderGraphBuilder("StandardPipeline");
    const RenderEngine::RenderGraph::BinarySemaphore image_available_semaphore{ ImageAcquireTask::image_available_semaphore_name };
    const RenderEngine::RenderGraph::TimelineSemaphore render_finished_semaphore{ ImageAcquireTask::render_finished_semaphore_name , 0, 2 };
    builder.registerSemaphore(image_available_semaphore);
    builder.registerSemaphore(render_finished_semaphore);

    ImageAcquireTask* image_acquire_task{ nullptr };
    {
        auto ui_renderer = std::make_unique<RenderEngine::UIRenderer>(_window_setup->getUiWindow(), _window_setup->getRenderingWindow().createRenderTarget(), 3);
        _ui_renderer = ui_renderer.get();
        auto forward_renderer = std::make_unique<RenderEngine::ForwardRenderer>(render_engine, _window_setup->getRenderingWindow().createRenderTarget());
        _forward_renderer = forward_renderer.get();

        auto image_acquire_task_ptr = std::make_unique<ImageAcquireTask>(_window_setup->getUiWindow(), _window_setup->getUiWindow().getDevice().getLogicalDevice(), _window_setup->getBackbufferCount());
        image_acquire_task = image_acquire_task_ptr.get();
        builder.addCpuNode("AcquireImage", std::move(image_acquire_task_ptr));
        builder.addRenderNode("ForwardRenderer", std::move(forward_renderer));
        builder.addRenderNode("ImguiRenderer", std::move(ui_renderer));
        builder.addPresentNode("Present", _window_setup->getUiWindow().getSwapChain());
        builder.addEmptyNode("End");
    }
    builder.addCpuSyncLink("AcquireImage", "ForwardRenderer")
        .signalOnGpu()
        .waitOnGpu(image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("ForwardRenderer", "ImguiRenderer")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("ImguiRenderer", "Present")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuAsyncLink("ImguiRenderer", "End")
        .signalOnGpu(render_finished_semaphore, 1, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_NONE);

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
        //_window_setup->update();
        render_engine.render();
        ApplicationContext::instance().updateInputEvents();
        ApplicationContext::instance().onFrameEnd();
    }
}

ParallelDemoApplication::~ParallelDemoApplication()
{
    if (_window_setup != nullptr)
    {
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
