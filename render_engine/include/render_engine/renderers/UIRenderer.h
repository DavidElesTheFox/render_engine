#pragma once

#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/window/SwapChain.h>

#include <volk.h>

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

struct ImGuiContext;
namespace RenderEngine
{
    class Window;

    class UIRenderer : public SingleColorOutputRenderer
    {
        struct ImGuiGlobals
        {
            std::set<ImGuiContext*> invalidContexts;
            std::atomic<VkDevice> loadedDevicePrototypes{ VK_NULL_HANDLE };
        };

        static ImGuiGlobals& getGlobals();
    public:
        static constexpr uint32_t kRendererId = 1u;

        UIRenderer(Window& window,
                   RenderTarget render_target,
                   uint32_t back_buffer_size);
        void onFrameBegin(uint32_t) override final {}
        void draw(uint32_t swap_chain_image_index) override final
        {
            draw(getFrameBuffer(swap_chain_image_index), getFrameData(swap_chain_image_index));
        }

        void setOnGui(std::function<void()> on_gui) { _on_gui = on_gui; }

        ~UIRenderer();

        SyncOperations getSyncOperations(uint32_t) final
        {
            return {};
        }
    private:

        static void registerDeletedContext(ImGuiContext* context);
        static bool isValidContext(ImGuiContext* context);

        std::vector<AttachmentInfo> reinitializeAttachments(const RenderTarget&) override final { return {}; }

        void draw(const VkFramebuffer& frame_buffer, FrameData& frame_data);
        void loadVulkanPrototypes();


        ImGuiContext* _imgui_context{ nullptr };
        ImGuiContext* _imgui_context_during_init{ nullptr };
        VkDescriptorPool _descriptor_pool{ VK_NULL_HANDLE };
        GLFWwindow* _window_handle{ nullptr };

        std::function<void()> _on_gui;
        PerformanceMarkerFactory _performance_markers;

    };
}