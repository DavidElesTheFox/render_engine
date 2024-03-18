#pragma once

#include <volk.h>

#include <render_engine/CommandContext.h>
#include <render_engine/Device.h>
#include <render_engine/RenderEngine.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/window/IWindow.h>
#include <render_engine/window/SwapChain.h>

#include <array>
#include <memory>
#include <optional>

#include <GLFW/glfw3.h>

struct ImGuiContext;
namespace RenderEngine
{
    class Device;
    class GpuResourceManager;
    class Window : public IWindow
    {
    public:

        static constexpr std::array<const char*, 1> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        Window(Device& device,
               std::unique_ptr<RenderEngine>&& render_engine,
               GLFWwindow* window,
               std::unique_ptr<SwapChain> swap_chain,
               std::shared_ptr<CommandContext>&& present_context);
        ~Window();

        void update() override final;
        bool isClosed() const override final { return _closed; }

        void registerRenderers(const std::vector<uint32_t>& renderer_ids) override final;

        Device& getDevice() override final { return _device; }
        void enableRenderdocCapture() override final;
        void disableRenderdocCapture() override final;
        RenderEngine& getRenderEngine()  override final { return *_render_engine; }

        GLFWwindow* getWindowHandle() { return _window; }
        template<typename T>
        T& getRendererAs(uint32_t renderer_id)
        {
            return static_cast<T&>(*_renderer_map.at(renderer_id));
        }
        AbstractRenderer* findRenderer(uint32_t renderer_id) const
        {
            auto it = _renderer_map.find(renderer_id);
            return it != _renderer_map.end() ? it->second : nullptr;
        }

        void registerTunnel(WindowTunnel& tunnel) override final;
        WindowTunnel* getTunnel() override final;
        TextureFactory& getTextureFactory() final;
    private:
        struct FrameData
        {
            FrameData(LogicalDevice& logical_device)
                : synch_render(logical_device)
                , submit_tracker(std::make_unique<QueueSubmitTracker>(logical_device))
            {}
            SyncObject synch_render;
            std::unique_ptr<QueueSubmitTracker> submit_tracker;
        };
        void initSynchronizationObjects();
        void handleEvents();
        void present();
        void present(FrameData& current_frame_data);
        void reinitSwapChain();
        void destroy();

        Device& _device;
        std::unique_ptr<RenderEngine> _render_engine;
        GLFWwindow* _window{ nullptr };
        std::unique_ptr<SwapChain> _swap_chain;
        std::shared_ptr<CommandContext> _present_context;

        std::vector<FrameData> _back_buffer;

        std::vector<std::unique_ptr<AbstractRenderer>> _renderers;
        std::unordered_map<uint32_t, AbstractRenderer*> _renderer_map;

        uint32_t _frame_counter{ 0 };
        uint32_t _presented_frame_counter{ 0 };
        void* _renderdoc_api{ nullptr };
        std::optional<uint32_t> _swap_chain_image_index;
        bool _new_swap_chain_image_is_required{ true };
        WindowTunnel* _tunnel{ nullptr };
        bool _closed = false;
    };
}