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

void DemoApplication::init()
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
    _scene_resources = demoSceneBuilder.buildSceneOfQuads(_assets,
                                                          *_scene,
                                                          *_texture_factory,
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
        vkDeviceWaitIdle(getRenderingWindow().getDevice().getLogicalDevice());
        vkDeviceWaitIdle(getUiWindow().getDevice().getLogicalDevice());
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
    RenderContext::initialize({ "VK_LAYER_KHRONOS_validation" }, std::move(renderers));
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
        _window_setup = std::make_unique<OffScreenWindowSetup>();
    }
    else
    {
        _window_setup = std::make_unique<SingleWindowSetup>();
    }
    _window_setup->getUiWindow().getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
        [&]
        {
            onGui();
        });

}

DemoApplication::SingleWindowSetup::SingleWindowSetup()
{
    using namespace RenderEngine;

    _window = RenderContext::context().getDevice(0).createWindow("Demo window", 2);
    _window->registerRenderers({ ForwardRenderer::kRendererId, UIRenderer::kRendererId });
}

DemoApplication::OffScreenWindowSetup::OffScreenWindowSetup()
{
    using namespace RenderEngine;
    {
        auto graphics_window = RenderContext::context().getDevice(0).createOffScreenWindow("Demo window (render)", 2);
        auto ui_window = RenderContext::context().getDevice(0).createWindow("Demo window", 2);
        _window_tunnel = std::make_unique<WindowTunnel>(std::move(graphics_window), std::move(ui_window));
    }
    _window_tunnel->getOriginWindow().registerRenderers({ ForwardRenderer::kRendererId });
    _window_tunnel->getDestinationWindow().registerRenderers({ ImageStreamRenderer::kRendererId, UIRenderer::kRendererId });
}
