#pragma once

#include <data_config.h>
#include <imgui.h>

#include <scene/Scene.h>
#include <scene/SceneRenderManager.h>

#include <render_engine/ParallelRenderEngine.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

#include <DemoSceneBuilder.h>
#include <ui/AssetBrowserUi.h>

#include <WindowSetup.h>

namespace RenderEngine
{
    class RenderTargetTextures
    {
    public:
        explicit RenderTargetTextures(std::vector<std::unique_ptr<Texture>>&& textures);
        ~RenderTargetTextures() = default;

        RenderTargetTextures(RenderTargetTextures&&) = delete;
        RenderTargetTextures(const RenderTargetTextures&) = delete;

        RenderTargetTextures& operator=(RenderTargetTextures&&) = delete;
        RenderTargetTextures& operator=(const RenderTargetTextures&) = delete;

        Texture& getTexture(uint32_t render_target_index)
        {
            return *_textures[render_target_index];
        }
        const Texture& getTexture(uint32_t render_target_index) const
        {
            return *_textures[render_target_index];
        }
        RenderTarget createRenderTarget() const;

        uint32_t getSize() const { return static_cast<uint32_t>(_textures.size()); }
    private:
        struct RenderTargetTextureData
        {
            std::unique_ptr<Texture> render_target_texture;
            std::unique_ptr<ITextureView> render_target_texture_view;
        };
        std::vector<std::unique_ptr<Texture>> _textures;
        std::vector<std::unique_ptr<ITextureView>> _texture_views;
    };
    class OffscreenSwapChain
    {
    public:
        OffscreenSwapChain(RefObj<Device> device, RefObj<RenderTargetTextures> render_target_textures);
        uint32_t acquireImageIndex(std::chrono::milliseconds timeout);
        void present(uint32_t render_target_index, const SyncOperations& sync_operations);
        void readback(uint32_t render_target_index, ImageStream& stream);
        Device& getDevice() { return *_device; }
        RenderTargetTextures& getRenderTargetTextures() { return *_render_target_textures; }
        const RenderTargetTextures& getRenderTargetTextures() const { return *_render_target_textures; }
    private:
        RefObj<Device> _device;
        RefObj<RenderTargetTextures> _render_target_textures;
        std::deque<uint32_t> _available_render_target_indexes;
        std::condition_variable _render_target_available;
        std::mutex _available_render_target_indexes_mutex;
    };
}
class ParallelDemoApplication
{
public:
    enum class RenderingType
    {
        Direct,
        Offscreen,
        OffscreenWithPresent
    };
    struct Description
    {
        uint32_t backbuffer_count;
        uint32_t thread_count;
        uint32_t window_width;
        uint32_t window_height;
        std::string window_title;
        RenderingType rendering_type;
    };

    explicit ParallelDemoApplication(Description description)
        : _description(std::move(description))
    {
        switch (description.rendering_type)
        {
            case RenderingType::Direct:
                _render_device_index = 0;
                _present_device_index = 0;
                break;
            case RenderingType::Offscreen:
                _render_device_index = 0;
                _present_device_index = 0;
                break;
            case RenderingType::OffscreenWithPresent:
                _render_device_index = 0;
                _present_device_index = 1;
                break;
        }
    }
    void init();

    void run();

    ~ParallelDemoApplication();

private:
    // TODO remove this class when old RenderEngine is dropped.
    class RenderEngineAccessor
    {
    public:
        RenderEngineAccessor() = default;
        void reset(std::unique_ptr<RenderEngine::ParallelRenderEngine> render_engine)
        {
            _render_engine = nullptr;
            _owned_render_engine = std::move(render_engine);
        }
        void reset(RenderEngine::ParallelRenderEngine* render_engine)
        {
            _render_engine = render_engine;
            _owned_render_engine = nullptr;
        }
        RenderEngine::ParallelRenderEngine* get() { return _owned_render_engine != nullptr ? _owned_render_engine.get() : _render_engine; }
        RenderEngine::ParallelRenderEngine* operator->() { return get(); }
    private:
        std::unique_ptr<RenderEngine::ParallelRenderEngine> _owned_render_engine;
        RenderEngine::ParallelRenderEngine* _render_engine{ nullptr };
    };

    void createRenderEngine();
    void createOffscreenRenderEngine();

    void createScene();

    void initializeRenderers();

    void onGui();
    void createWindowSetup();
    void createOffscreenWindow();

    RenderEngine::Window& getUiWindow() { return *_window; }

    Assets::AssetDatabase _assets;

    std::unique_ptr<RenderEngine::Window> _window;
    // TODO create offscreen window from these two data.
    std::unique_ptr<RenderEngine::RenderTargetTextures> _offscreen_textures;
    std::unique_ptr<RenderEngine::OffscreenSwapChain> _offscreen_swap_chain;

    std::unique_ptr<Scene::Scene> _scene;

    std::unique_ptr<Scene::SceneRenderManager> _render_manager;
    std::unique_ptr<Ui::AssetBrowserUi> _asset_browser;
    DemoSceneBuilder::CreationResult _scene_resources;
    RenderEngine::UIRenderer* _ui_renderer{ nullptr };
    RenderEngine::ForwardRenderer* _forward_renderer{ nullptr };
    Description _description;
    uint32_t _present_device_index{ 0 };
    uint32_t _render_device_index{ 0 };
    RenderEngineAccessor _render_engine;
    RenderEngineAccessor _present_render_engine;
};
