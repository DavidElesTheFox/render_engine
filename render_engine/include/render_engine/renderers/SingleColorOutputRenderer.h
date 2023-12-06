#pragma once

#include <render_engine/CommandPoolFactory.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/window/Window.h>

#include <memory>
#include <render_engine/resources/Buffer.h>

namespace RenderEngine
{
    class SingleColorOutputRenderer : public AbstractRenderer
    {
    public:
        explicit SingleColorOutputRenderer(IWindow& parent);
        ~SingleColorOutputRenderer() override;
        std::vector<VkCommandBuffer> getCommandBuffers(uint32_t image_index) override final
        {
            if (skipDrawCall(image_index))
            {
                return{};
            }
            return { getFrameData(image_index).command_buffer };
        }
    protected:
        struct FrameData
        {
            VkCommandBuffer command_buffer;
        };
        virtual bool skipDrawCall(uint32_t image_index) const { return false; }
        void initializeRendererOutput(const RenderTarget& render_target,
                                      VkRenderPass render_pass,
                                      size_t back_buffer_size);
        void destroyRenderOutput();
        IWindow& getWindow() { return _window; }
        VkDevice getLogicalDevice() { return _window.getDevice().getLogicalDevice(); }
        FrameData& getFrameData(uint32_t image_index)
        {
            return _back_buffer[image_index];
        }
        VkRenderPass getRenderPass() { return _render_pass; }
        VkFramebuffer getFrameBuffer(uint32_t swap_chain_image_index) { return _frame_buffers[swap_chain_image_index]; }
        const VkRect2D& getRenderArea() const { return _render_area; }
    private:
        void createFrameBuffers(const RenderTarget&);
        bool createFrameBuffer(const RenderTarget& render_target, uint32_t frame_buffer_index);
        void createCommandBuffer();
        void resetFrameBuffers();
        void beforeReinit() override final;
        void finalizeReinit(const RenderTarget& render_target) override final;

        IWindow& _window;
        std::vector<VkFramebuffer> _frame_buffers;
        CommandPoolFactory::CommandPool _command_pool;
        std::vector<FrameData> _back_buffer;
        VkRenderPass _render_pass{ VK_NULL_HANDLE };
        VkRect2D _render_area{};
    };
}