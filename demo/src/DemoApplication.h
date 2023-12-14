#pragma once
#include <data_config.h>
#include <imgui.h>
#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/Device.h>
#include <render_engine/RenderContext.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/resources/Texture.h>
#include <render_engine/resources/UniformBinding.h>
#include <render_engine/window/WindowTunnel.h>

#include <scene/Scene.h>
#include <scene/SceneRenderManager.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

#include <assets/AssetDatabase.h>
#include <assets/NoLitMaterial.h>
#include <DemoSceneBuilder.h>
#include <ui/AssetBrowserUi.h>

#include <WindowSetup.h>

class DemoApplication
{
public:
    void init();

    void run();

    ~DemoApplication();

private:

    void createScene();

    void initializeRenderers();

    void onGui();
    void createWindowSetup();

    RenderEngine::IWindow& getRenderingWindow() { return _window_setup->getRenderingWindow(); }
    RenderEngine::Window& getUiWindow() { return _window_setup->getUiWindow(); }

    Assets::AssetDatabase _assets;

    std::unique_ptr<IWindowSetup> _window_setup;

    std::unique_ptr<Scene::Scene> _scene;

    std::unique_ptr<Scene::SceneRenderManager> _render_manager;
    std::unique_ptr<Ui::AssetBrowserUi> _asset_browser;
    DemoSceneBuilder::CreationResult _scene_resources;
    std::unique_ptr<RenderEngine::TextureFactory> _texture_factory;
};
