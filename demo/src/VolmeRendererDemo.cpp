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

void VolumeRendererDemo::init()
{
    using namespace RenderEngine;

    initializeRenderers();
    createWindowSetup();

    createScene();
    _texture_factory = RenderContext::context().getDevice(0).createTextureFactory(
        getRenderingWindow().getTransferEngine(),
        { getRenderingWindow().getTransferEngine().getQueueFamilyIndex(), getRenderingWindow().getRenderEngine().getQueueFamilyIndex() }
    );
    DemoSceneBuilder demoSceneBuilder;
    _scene_resources = demoSceneBuilder.buildVolumetricScene(_assets,
                                                             *_scene,
                                                             *_texture_factory,
                                                             getRenderingWindow().getRenderEngine());

    ApplicationContext::instance().init(_scene.get(), getUiWindow().getWindowHandle());

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), getRenderingWindow());

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
        vkDeviceWaitIdle(getRenderingWindow().getDevice().getLogicalDevice());
        vkDeviceWaitIdle(getUiWindow().getDevice().getLogicalDevice());
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
                                [](auto& window, const auto& render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
                                {
                                    return std::make_unique<UIRenderer>(dynamic_cast<Window&>(window), render_target, back_buffer_count, previous_renderer == nullptr);
                                });
    renderers->registerRenderer(ForwardRenderer::kRendererId,
                                [](auto& window, const auto& render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
                                {
                                    return std::make_unique<ForwardRenderer>(window, render_target, has_next);
                                });
    renderers->registerRenderer(ImageStreamRenderer::kRendererId,
                                [](IWindow& window, const auto& render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
                                {
                                    assert(window.getTunnel() != nullptr && "No tunnel defined for the given window. Cannot connect with its origin's stream");
                                    return std::make_unique<ImageStreamRenderer>(window, window.getTunnel()->getOriginWindow().getImageStream(), render_target, back_buffer_count, has_next);

                                });
    renderers->registerRenderer(VolumeRenderer::kRendererId,
                                [](IWindow& window, const auto& render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
                                {
                                    return std::make_unique<VolumeRenderer>(window, render_target, has_next);
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


