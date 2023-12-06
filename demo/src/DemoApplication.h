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


class DemoApplication
{
public:
    void init();

    void run();

    ~DemoApplication();

private:
    class IWindowSetup
    {
    public:
        virtual ~IWindowSetup() = default;
        virtual RenderEngine::IWindow& getRenderingWindow() = 0;
        virtual RenderEngine::Window& getUiWindow() = 0;
        virtual void update() = 0;
    };

    class SingleWindowSetup : public IWindowSetup
    {
    public:
        SingleWindowSetup();
        ~SingleWindowSetup() override = default;
        RenderEngine::IWindow& getRenderingWindow() override { return *_window; }
        RenderEngine::Window& getUiWindow() override { return *_window; };
        void update() override { _window->update(); }
    private:
        std::unique_ptr<RenderEngine::Window> _window;
    };

    class OffScreenWindowSetup : public IWindowSetup
    {
    public:
        OffScreenWindowSetup();
        ~OffScreenWindowSetup() override = default;
        RenderEngine::IWindow& getRenderingWindow() override { return _window_tunnel->getOriginWindow(); }
        RenderEngine::Window& getUiWindow() override
        {
            return static_cast<RenderEngine::Window&>(_window_tunnel->getDestinationWindow());
        }
        void update() override { _window_tunnel->update(); }
    private:
        std::unique_ptr<RenderEngine::WindowTunnel> _window_tunnel;
    };
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
