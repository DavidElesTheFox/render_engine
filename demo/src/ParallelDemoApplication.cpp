#include "ParallelDemoApplication.h"

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

    class ImageAcquireTask : public rg::CpuNode::ICpuTask
    {
    public:
        constexpr static auto render_finished_semaphore_name = "render-finished";
        constexpr static auto image_available_semaphore_name = "image-available";
        ImageAcquireTask(RenderEngine::Window& window, RenderEngine::LogicalDevice& logical_device, uint32_t back_buffer_count)
            : _window(window)
            , _backbuffer_count(back_buffer_count)
            , _logical_device(logical_device)
        {}
        void run(rg::Job::ExecutionContext& execution_context) final
        {
            if (glfwGetWindowAttrib(_window.getWindowHandle(), GLFW_ICONIFIED) == GLFW_TRUE)
            {
                return;
            }
            if (execution_context.hasRenderTargetIndex())
            {
                return;
            }
            {
                /*
                * This is tricky:
                * There are renders ongoing on the back buffers. Each back buffer has its own semaphore to trigger when the drawing are finished
                * Its gonna be the available_index. Its semaphore will be used to query the next back buffer.
                */

                uint32_t available_index = RenderEngine::SyncObject::SharedOperations::from(execution_context.getAllSyncObject()).waitAnyOfSemaphores(render_finished_semaphore_name, 1);
                auto [call_result, image_index] = execution_context.getSyncObject(available_index).acquireNextSwapChainImage(_logical_device,
                                                                                                                             _window.getSwapChain().getDetails().swap_chain,
                                                                                                                             image_available_semaphore_name);
                switch (call_result)
                {
                    case VK_ERROR_OUT_OF_DATE_KHR:
                    case VK_SUBOPTIMAL_KHR:
                        _window.reinitSwapChain();
                        return;
                    case VK_SUCCESS:
                        break;
                    default:
                        throw std::runtime_error("Failed to query swap chain image");
                }
                /*
                * If somehow the next image is not the one that is finished we need to wait it until it is not finished.
                * Like having 3 ongoing render
                *  [0] : rendering
                *  [1] : finished
                *  [2] : rendering
                * Theoretically, it can happen that the available index is '1' but the acquire next image returns with 2 (even due to a bug or whatever) then
                * it would be a pity if we mess up the two back buffers. So let's wait for it.
                * TODO: Add warning message
                */
                if (image_index != available_index)
                {
                    execution_context.getSyncObject(image_index).waitSemaphore(render_finished_semaphore_name, 1);
                }
                execution_context.setRenderTargetIndex(image_index);

                execution_context.getSyncObjectUpdateData().requestTimelineSemaphoreShift(&execution_context.getSyncObject(image_index), render_finished_semaphore_name);
                execution_context.getSyncObjectUpdateData().requestTimelineSemaphoreShift(&execution_context.getSyncObject(available_index), render_finished_semaphore_name);
            }
        }
        bool isActive() const final { return true; }
    private:
        RenderEngine::Window& _window;
        RenderEngine::LogicalDevice& _logical_device;
        uint32_t _backbuffer_count{ 0 };
    };
}

void ParallelDemoApplication::createRenderEngine()
{
    auto& render_engine = static_cast<RenderEngine::ParallelRenderEngine&>(_window_setup->getRenderingWindow().getRenderEngine());

    RenderEngine::RenderGraph::RenderGraphBuilder builder = render_engine.createRenderGraphBuilder("StandardPipeline");
    const RenderEngine::RenderGraph::BinarySemaphore image_available_semaphore{ ImageAcquireTask::image_available_semaphore_name };
    const RenderEngine::RenderGraph::TimelineSemaphore render_finished_semaphore{ ImageAcquireTask::render_finished_semaphore_name , 1, 2 };
    builder.registerSemaphore(image_available_semaphore);
    builder.registerSemaphore(render_finished_semaphore);

    ImageAcquireTask* image_acquire_task{ nullptr };
    {
        auto image_acquire_task_ptr = std::make_unique<ImageAcquireTask>(_window_setup->getUiWindow(), _window_setup->getUiWindow().getDevice().getLogicalDevice(), _window_setup->getBackbufferCount());
        image_acquire_task = image_acquire_task_ptr.get();
        builder.addCpuNode("AcquireImage", std::move(image_acquire_task_ptr));
        builder.addRenderNode("ForwardRenderer", std::make_unique<RenderEngine::ForwardRenderer>(render_engine, _window_setup->getRenderingWindow().createRenderTarget()));
        builder.addRenderNode("ImguiRenderer", std::make_unique<RenderEngine::UIRenderer>(_window_setup->getUiWindow(), _window_setup->getRenderingWindow().createRenderTarget(), 3));
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

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), getRenderingWindow());

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
    while (getUiWindow().isClosed() == false)
    {
        ApplicationContext::instance().onFrameBegin();
        _window_setup->update();
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
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation" };
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
    _window_setup->getUiWindow().getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
        [&]
        {
            onGui();
        });
}
