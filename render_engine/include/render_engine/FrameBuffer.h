#pragma once

#include <render_engine/resources/Texture.h>

#include <vector>

#include <volk.h>

namespace RenderEngine
{
    class FrameBuffer;
    class FrameBufferBuilder
    {
    public:
        friend class FrameBuffer;
        FrameBufferBuilder(VkRenderPass render_pass,
                           std::vector<VkAttachmentDescription> attachment_description)
            : _render_pass(render_pass)
            , _attachment_description(std::move(attachment_description))
        {}
        void setAttachment(uint32_t id, ITextureView* texture_view);
        FrameBuffer build(uint32_t width, uint32_t height, LogicalDevice& logical_device);
    private:
        VkRenderPass _render_pass;
        std::vector<VkAttachmentDescription> _attachment_description;
        std::vector<ITextureView*> _attachments;
    };

    class FrameBuffer
    {
    public:
        FrameBuffer(uint32_t width, uint32_t height, FrameBufferBuilder origin, VkFramebuffer frame_buffer)
            : _width(width)
            , _height(height)
            , _origin(std::move(origin))
            , _frame_buffer(frame_buffer)
        {}
        FrameBuffer(FrameBuffer&&) = default;
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(FrameBuffer&&) = default;
        FrameBuffer& operator=(const FrameBuffer&) = delete;
        VkFramebuffer get() { return _frame_buffer; }
        Texture& getAttachmentTexture(uint32_t index) { return _origin._attachments[index]->getTexture(); }
    private:
        uint32_t _width{ 0 };
        uint32_t _height{ 0 };
        FrameBufferBuilder _origin;
        VkFramebuffer _frame_buffer{ VK_NULL_HANDLE };
    };
}