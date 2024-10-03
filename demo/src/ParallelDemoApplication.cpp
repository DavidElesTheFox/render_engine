#include "ParallelDemoApplication.h"

#include <render_engine/containers/IndexSet.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/rendergraph/cputasks/ImageAquireTask.h>
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


void ParallelDemoApplication::createRenderEngine()
{
    namespace rg = RenderEngine::RenderGraph;
    auto& render_engine = static_cast<RenderEngine::ParallelRenderEngine&>(_window_setup->getRenderingWindow().getRenderEngine());

    rg::RenderGraphBuilder builder = render_engine.createRenderGraphBuilder("StandardPipeline");
    const rg::BinarySemaphore image_available_semaphore{ rg::ImageAcquireTask::image_available_semaphore_name };
    builder.registerSemaphore(image_available_semaphore);

    rg::ImageAcquireTask* image_acquire_task{ nullptr };
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
        auto image_acquire_task_ptr = std::make_unique<rg::ImageAcquireTask>(_window_setup->getUiWindow(),
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
