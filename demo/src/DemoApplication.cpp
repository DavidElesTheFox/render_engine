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
    createWindow();

    createScene();
    _texture_factory = RenderContext::context().getDevice(0).createTextureFactory(
        _window->getTransferEngine(),
        { _window->getTransferEngine().getQueueFamilyIndex(), _window->getRenderEngine().getQueueFamilyIndex() }
    );
    DemoSceneBuilder demoSceneBuilder;
    _scene_resources = demoSceneBuilder.buildSceneOfQuads(_assets, *_scene, *_texture_factory, _window->getRenderEngine());

    ApplicationContext::instance().init(_scene.get(), _window->getWindowHandle());

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), *_window);

    _render_manager->registerMeshesForRender();

    _asset_browser = std::make_unique<Ui::AssetBrowserUi>(_assets, *_scene);
}

void DemoApplication::run()
{
    while (_window->isClosed() == false)
    {
        _window->update();
        ApplicationContext::instance().updateInputEvents();
    }
}

DemoApplication::~DemoApplication()
{
    // finish all rendering before destroying anything
    vkDeviceWaitIdle(_window->getDevice().getLogicalDevice());
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
                                [&](auto& window, const auto& render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
                                {
                                    return std::make_unique<ForwardRenderer>(window, render_target, has_next);
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

void DemoApplication::createWindow()
{
    using namespace RenderEngine;

    _window = RenderContext::context().getDevice(0).createWindow("Secondary Window", 3);
    _window->registerRenderers({ ForwardRenderer::kRendererId, UIRenderer::kRendererId });

    _window->getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
        [&]
        {
            onGui();
        });
}


