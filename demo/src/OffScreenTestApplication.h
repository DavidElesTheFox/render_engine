#pragma once

#include <render_engine/window/OffScreenWindow.h>


#include <assets/AssetDatabase.h>
#include <DemoSceneBuilder.h>
#include <scene/Scene.h>
#include <scene/SceneRenderManager.h>
#include <ui/AssetBrowserUi.h>


class OffScreenTestApplication
{
public:
    void init();

    void run();

    ~OffScreenTestApplication();

private:
    void createScene();

    void initializeRenderers();

    void createWindow();

    Assets::AssetDatabase _assets;

    std::unique_ptr<RenderEngine::OffScreenWindow> _window;

    std::unique_ptr<Scene::Scene> _scene;

    std::unique_ptr<Scene::SceneRenderManager> _render_manager;
    std::unique_ptr<Ui::AssetBrowserUi> _asset_browser;
    DemoSceneBuilder::CreationResult _scene_resources;
    std::unique_ptr<RenderEngine::TextureFactory> _texture_factory;
    bool _save_output = false;
};
