#include <render_engine/FrameBuffer.h>

namespace RenderEngine
{
    void FrameBufferBuilder::setAttachment(uint32_t id, ITextureView* texture_view)
    {
        _attachments[id] = texture_view;
    }
    FrameBuffer FrameBufferBuilder::build(uint32_t width, uint32_t height, LogicalDevice& logical_device)
    {
        std::vector<VkImageView> attachments;
        std::ranges::transform(_attachments, std::back_inserter(attachments),
                               [](ITextureView* view) { return view->getImageView(); });
        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = _render_pass;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.width = width;
        create_info.height = height;
        create_info.layers = 1; // TODO: add multiview support

        VkFramebuffer frame_buffer{ VK_NULL_HANDLE };
        if (logical_device->vkCreateFramebuffer(*logical_device, &create_info, VK_NULL_HANDLE, &frame_buffer) == false)
        {
            throw std::runtime_error("Can't create framebuffer");
        }

        return FrameBuffer(width, height, *this, frame_buffer);
    }
}
