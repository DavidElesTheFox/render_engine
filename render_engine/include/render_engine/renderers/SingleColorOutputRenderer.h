#pragma once

#include <render_engine/CommandPoolFactory.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/window/Window.h>

#include <memory>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

namespace RenderEngine
{
    class SingleColorOutputRenderer : public AbstractRenderer
    {
    public:
        explicit SingleColorOutputRenderer(IRenderEngine& render_engine);
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
        struct AttachmentInfo
        {
            std::vector<ITextureView*> attachments;
        };
        virtual bool skipDrawCall(uint32_t) const { return false; }
        void initializeRendererOutput(RenderTarget& render_target,
                                      VkRenderPass render_pass,
                                      size_t back_buffer_size,
                                      bool create_internal_command_buffers,
                                      const std::vector<AttachmentInfo>& render_pass_attachments = {});
        void destroyRenderOutput();
        LogicalDevice& getLogicalDevice() const { return _render_engine.getDevice().getLogicalDevice(); }
        VkPhysicalDevice getPhysicalDevice() const { return _render_engine.getDevice().getPhysicalDevice(); }
        FrameData& getFrameData(uint32_t image_index)
        {
            return _back_buffer[image_index];
        }
        VkRenderPass getRenderPass() { return _render_pass; }
        VkFramebuffer getFrameBuffer(uint32_t swap_chain_image_index) { return _frame_buffers[swap_chain_image_index]; }
        const VkRect2D& getRenderArea() const { return _render_area; }
        TextureFactory& getTextureFactory() { return _render_engine.getDevice().getTextureFactory(); }
        IRenderEngine& getRenderEngine() { return _render_engine; }
    private:
        void createFrameBuffers(const RenderTarget&, const std::vector<AttachmentInfo>& render_pass_attachments);
        bool createFrameBuffer(const RenderTarget& render_target, uint32_t frame_buffer_index, const AttachmentInfo& render_pass_attachments);
        void createCommandBuffer();
        void resetFrameBuffers();
        void beforeReinit() override final;
        void finalizeReinit(const RenderTarget& render_target) override final;
        void initializeRenderTargetCommandContext(RenderTarget& render_target);

        virtual std::vector<AttachmentInfo> reinitializeAttachments(const RenderTarget& render_target) = 0;

        IRenderEngine& _render_engine;
        std::vector<VkFramebuffer> _frame_buffers;
        std::vector<FrameData> _back_buffer;
        VkRenderPass _render_pass{ VK_NULL_HANDLE };
        VkRect2D _render_area{};
    };
}