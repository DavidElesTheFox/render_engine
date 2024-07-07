#include "VolumeRendererDemo.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/vec4.hpp>

#include <iostream>

#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/renderers/VolumeRenderer.h>
#include <render_engine/resources/PushConstantsUpdater.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>

#include <ApplicationContext.h>

#include <assets/BillboardMaterial.h>

#include <demo/resource_config.h>

#include <DemoSceneBuilder.h>
#include <DeviceSelector.h>

void VolumeRendererDemo::init(bool use_ao)
{
    using namespace RenderEngine;

    initializeRenderers();
    createWindowSetup();

    createScene();
    DemoSceneBuilder demoSceneBuilder;
    _scene_resources = demoSceneBuilder.buildVolumetricScene(_assets,
                                                             *_scene,
                                                             getRenderingWindow().getDevice(),
                                                             getRenderingWindow().getRenderEngine(),
                                                             use_ao);

    ApplicationContext::instance().init(_scene.get(), getUiWindow().getWindowHandle());

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(),
                                                                  static_cast<ForwardRenderer*>(getRenderingWindow().findRenderer(ForwardRenderer::kRendererId)),
                                                                  static_cast<VolumeRenderer*>(getRenderingWindow().findRenderer(VolumeRenderer::kRendererId)));
    _render_manager->registerMeshesForRender();

    _asset_browser = std::make_unique<Ui::AssetBrowserUi>(_assets, *_scene);
}

void VolumeRendererDemo::run()
{
    while (getUiWindow().isClosed() == false)
    {
        ApplicationContext::instance().onFrameBegin();
        _window_setup->update();
        ApplicationContext::instance().updateInputEvents();
        ApplicationContext::instance().onFrameEnd();
    }
}

VolumeRendererDemo::~VolumeRendererDemo()
{
    if (_window_setup != nullptr)
    {
        // finish all rendering before destroying anything
        getRenderingWindow().getDevice().waitIdle();
        getUiWindow().getDevice().waitIdle();
    }
}

void VolumeRendererDemo::createScene()
{
    Scene::Scene::SceneSetup scene_setup{
        .up = glm::vec3(0.0f, 1.0f, 0.0f),
        .forward = glm::vec3(0.0f, 0.0f, -1.0f)
    };

    _scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
}

void VolumeRendererDemo::initializeRenderers()
{
    using namespace RenderEngine;

    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
    renderers->registerRenderer(UIRenderer::kRendererId,
                                [](auto& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
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
                                    return std::make_unique<UIRenderer>(dynamic_cast<Window&>(window), render_target, back_buffer_count, true);
                                });
    renderers->registerRenderer(ForwardRenderer::kRendererId,
                                [](auto& window, auto render_target, uint32_t, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<ForwardRenderer>(window.getRenderEngine(), render_target, true);
                                });
    renderers->registerRenderer(ImageStreamRenderer::kRendererId,
                                [](IWindow& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    assert(window.getTunnel() != nullptr && "No tunnel defined for the given window. Cannot connect with its origin's stream");
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<ImageStreamRenderer>(window.getRenderEngine(), window.getTunnel()->getOriginWindow().getImageStream(), render_target, back_buffer_count, true);

                                });
    renderers->registerRenderer(VolumeRenderer::kRendererId,
                                [](IWindow& window, auto render_target, uint32_t, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<VolumeRenderer>(window.getRenderEngine(), render_target, true);
                                });

    DeviceSelector device_selector;
    RenderContext::InitializationInfo init_info{};
    init_info.app_info.apiVersion = VK_API_VERSION_1_3;
    init_info.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    init_info.app_info.applicationVersion = 0;
    init_info.app_info.engineVersion = 0;
    init_info.app_info.pApplicationName = "VolumeRenderer";
    init_info.app_info.pEngineName = "FoxEngine";
    init_info.enable_validation_layers = true;
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation" };
    init_info.renderer_factory = std::move(renderers);
    init_info.device_selector = [&](const DeviceLookup& lookup) ->VkPhysicalDevice { return device_selector.askForDevice(lookup); };
    init_info.queue_family_selector = [&](const DeviceLookup::DeviceInfo& info) { return device_selector.askForQueueFamilies(info); };
    init_info.instance_extensions =
    {
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME
    };
    init_info.device_extensions =
    {
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
    };
    RenderContext::initialize(std::move(init_info));
}



void VolumeRendererDemo::onGui()
{
    _asset_browser->onGui();
    if (_scene->getActiveCamera())
    {
        _scene->getActiveCamera()->onGui();
    }
    ApplicationContext::instance().onGui();
}

void VolumeRendererDemo::createWindowSetup()
{
    using namespace RenderEngine;
    constexpr bool use_offscreen_rendering = false;

    if constexpr (use_offscreen_rendering)
    {
        _window_setup = std::make_unique<OffScreenWindowSetup>(std::vector{ VolumeRenderer::kRendererId });
    }
    else
    {
        _window_setup = std::make_unique<SingleWindowSetup>(std::vector{ VolumeRenderer::kRendererId });
    }
    _window_setup->getUiWindow().getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
        [&]
        {
            onGui();
        });

}


