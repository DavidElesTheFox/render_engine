#pragma once

#include <render_engine/CommandPoolFactory.h>

#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/window/SwapChain.h>

#include <volk.h>

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
struct ImGuiContext;
namespace RenderEngine
{
    class Window;

    class UIRenderer : public SingleColorOutputRenderer
    {
        static std::set<ImGuiContext*> kInvalidContexts;
    public:
        static constexpr uint32_t kRendererId = 1u;

        UIRenderer(Window& window,
                   const RenderTarget& render_target,
                   uint32_t back_buffer_size,
                   bool first_renderer);
        void onFrameBegin(uint32_t frame_number) override final {}
        void draw(uint32_t swap_chain_image_index) override final
        {
            draw(getFrameBuffer(swap_chain_image_index), getFrameData(swap_chain_image_index));
        }

        void setOnGui(std::function<void()> on_gui) { _on_gui = on_gui; }

        ~UIRenderer();

        std::vector<VkSemaphoreSubmitInfo> getWaitSemaphores(uint32_t frame_number) override final
        {
            return {};
        }
    private:


        static void registerDeletedContext(ImGuiContext* context);
        static bool isValidContext(ImGuiContext* context);

        std::vector<AttachmentInfo> reinitializeAttachments(const RenderTarget& render_target) override final { return {}; }

        void draw(const VkFramebuffer& frame_buffer, FrameData& frame_data);

        ImGuiContext* _imgui_context{ nullptr };
        ImGuiContext* _imgui_context_during_init{ nullptr };
        VkDescriptorPool _descriptor_pool{ VK_NULL_HANDLE };
        GLFWwindow* _window_handle{ nullptr };

        std::function<void()> _on_gui;

    };
}