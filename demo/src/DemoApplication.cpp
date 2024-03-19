#include "DemoApplication.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/vec4.hpp>

#include <iostream>

#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/resources/PushConstantsUpdater.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>

#include <ApplicationContext.h>

#include <assets/BillboardMaterial.h>

#include <demo/resource_config.h>

#include <DemoSceneBuilder.h>
#include <DeviceSelector.h>

void DemoApplication::init()
{
    using namespace RenderEngine;

    initializeRenderers();
    createWindowSetup();

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

void DemoApplication::run()
{
    while (getUiWindow().isClosed() == false)
    {
        ApplicationContext::instance().onFrameBegin();
        _window_setup->update();
        ApplicationContext::instance().updateInputEvents();
        ApplicationContext::instance().onFrameEnd();
    }
}

DemoApplication::~DemoApplication()
{
    if (_window_setup != nullptr)
    {
        // finish all rendering before destroying anything
        getRenderingWindow().getDevice().waitIdle();
        getUiWindow().getDevice().waitIdle();
    }
}

void DemoApplication::createScene()
{
    Scene::Scene::SceneSetup scene_setup{
        .up = glm::vec3(0.0f, 1.0f, 0.0f),
        .forward = glm::vec3(0.0f, 0.0f, -1.0f)
    };

    _scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
}

void DemoApplication::initializeRenderers()
{
    using namespace RenderEngine;

    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
    renderers->registerRenderer(UIRenderer::kRendererId,
                                [](auto& window, RenderTarget render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
                                {
                                    const bool first_renderer = previous_renderer == nullptr;
                                    if (first_renderer)
                                    {
                                        render_target = render_target.clone()
                                            .changeLoadOperation(VK_ATTACHMENT_LOAD_OP_CLEAR)
                                            .changeInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
                                    }
                                    else
                                    {
                                        render_target = render_target.clone()
                                            .changeLoadOperation(VK_ATTACHMENT_LOAD_OP_LOAD)
                                            .changeInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<UIRenderer>(dynamic_cast<Window&>(window), render_target, back_buffer_count);
                                });
    renderers->registerRenderer(ForwardRenderer::kRendererId,
                                [](IWindow& window, RenderTarget render_target, uint32_t, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<ForwardRenderer>(window.getRenderEngine(), render_target);
                                });
    renderers->registerRenderer(ImageStreamRenderer::kRendererId,
                                [](IWindow& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    assert(window.getTunnel() != nullptr && "No tunnel defined for the given window. Cannot connect with its origin's stream");
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<ImageStreamRenderer>(window.getRenderEngine(), window.getTunnel()->getOriginWindow().getImageStream(), render_target, back_buffer_count);

                                });

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



void DemoApplication::onGui()
{
    _asset_browser->onGui();
    if (_scene->getActiveCamera())
    {
        _scene->getActiveCamera()->onGui();
    }
    ApplicationContext::instance().onGui();
}

void DemoApplication::createWindowSetup()
{
    using namespace RenderEngine;
    constexpr bool use_offscreen_rendering = true;

    if constexpr (use_offscreen_rendering)
    {
        _window_setup = std::make_unique<OffScreenWindowSetup>(std::vector{ ForwardRenderer::kRendererId });
    }
    else
    {
        _window_setup = std::make_unique<SingleWindowSetup>(std::vector{ ForwardRenderer::kRendererId });
    }
    _window_setup->getUiWindow().getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
        [&]
        {
            onGui();
        });

}


