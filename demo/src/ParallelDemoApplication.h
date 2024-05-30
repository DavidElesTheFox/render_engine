#pragma once

#include <data_config.h>
#include <imgui.h>

#include <scene/Scene.h>
#include <scene/SceneRenderManager.h>

#include <render_engine/ParallelRenderEngine.h>

#include <DemoSceneBuilder.h>
#include <ui/AssetBrowserUi.h>

#include <WindowSetup.h>

class ParallelDemoApplication
{
public:
    void init();

    void run();

    ~ParallelDemoApplication();

private:

    void createRenderEngine();

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
};
