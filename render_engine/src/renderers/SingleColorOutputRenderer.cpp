#include <render_engine/renderers/SingleColorOutputRenderer.h>

#include <render_engine/resources/RenderTarget.h>
#include <render_engine/window/Window.h>

namespace RenderEngine
{
    SingleColorOutputRenderer::SingleColorOutputRenderer(IWindow& window)
        : _window(window)
    {

    }
    SingleColorOutputRenderer::~SingleColorOutputRenderer()
    {
        destroyRenderOutput();
    }
    void SingleColorOutputRenderer::initializeRendererOutput(RenderTarget& render_target,
                                                             VkRenderPass render_pass,
                                                             size_t back_buffer_size,
                                                             const std::vector<AttachmentInfo>& render_pass_attachments)
    {
        _render_pass = render_pass;
        _back_buffer.resize(back_buffer_size);

        _render_area.offset = { 0, 0 };
        _render_area.extent = render_target.getExtent();
        createFrameBuffers(render_target, render_pass_attachments);
        createCommandBuffer();
        initializeRenderTargetCommandContext(render_target);

    }

    void SingleColorOutputRenderer::initializeRenderTargetCommandContext(RenderTarget& render_target)
    {
        for (uint32_t i = 0; i < render_target.getTexturesCount(); ++i)
        {
            if (render_target.getTextureView(i).getTexture().getResourceState().command_context.expired())
            {
                render_target.getTextureView(i).getTexture().setInitialCommandContext(_window.getRenderEngine().getCommandContext().getWeakReference());
            }
            else
            {
                assert(render_target.getTextureView(i).getTexture().getResourceState().command_context.lock()->isCompatibleWith(_window.getRenderEngine().getCommandContext())
                       && "The render target current context needs to be compatible with the current render engine command context");
            }
        }
    }

    void SingleColorOutputRenderer::destroyRenderOutput()
    {
        auto& logical_device = _window.getDevice().getLogicalDevice();

        resetFrameBuffers();

        logical_device->vkDestroyRenderPass(*logical_device, _render_pass, nullptr);
    }
    void SingleColorOutputRenderer::createFrameBuffers(const RenderTarget& render_target, const std::vector<AttachmentInfo>& render_pass_attachments)
    {
        auto& logical_device = _window.getDevice().getLogicalDevice();

        _frame_buffers.resize(render_target.getTexturesCount());
        for (uint32_t i = 0; i < _frame_buffers.size(); ++i)
        {
            const AttachmentInfo attachment_info = render_pass_attachments.empty() ? AttachmentInfo{} : render_pass_attachments[i];
            if (createFrameBuffer(render_target, i, attachment_info) == false)
            {
                for (uint32_t j = 0; j < i; ++j)
                {
                    logical_device->vkDestroyFramebuffer(*logical_device, _frame_buffers[j], nullptr);
                }
                _frame_buffers.clear();
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    bool SingleColorOutputRenderer::createFrameBuffer(const RenderTarget& render_target,
                                                      uint32_t frame_buffer_index,
                                                      const AttachmentInfo& render_pass_attachment)
    {
        auto& logical_device = _window.getDevice().getLogicalDevice();

        std::vector<VkImageView> attachments = {
                render_target.getTextureView(frame_buffer_index).getImageView()
        };
        std::ranges::transform(render_pass_attachment.attachments, std::back_inserter(attachments),
                               [](ITextureView* view) { return view->getImageView(); });

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = render_target.getWidth();
        framebuffer_info.height = render_target.getHeight();
        framebuffer_info.layers = 1;

        return logical_device->vkCreateFramebuffer(*logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) == VK_SUCCESS;
    }
    void SingleColorOutputRenderer::createCommandBuffer()
    {
        std::vector<VkCommandBuffer> command_buffers = _window.getRenderEngine().getCommandContext().createCommandBuffers(static_cast<uint32_t>(_back_buffer.size()),
                                                                                                                          CommandContext::Usage::MultipleSubmit);
        for (uint32_t i = 0; i < _back_buffer.size(); ++i)
        {
            _back_buffer[i].command_buffer = command_buffers[i];
        }
    }
    void SingleColorOutputRenderer::resetFrameBuffers()
    {
        auto& logical_device = _window.getDevice().getLogicalDevice();

        for (auto framebuffer : _frame_buffers)
        {
            logical_device->vkDestroyFramebuffer(*logical_device, framebuffer, nullptr);
        }
    }
    void SingleColorOutputRenderer::beforeReinit()
    {
        resetFrameBuffers();
    }
    void SingleColorOutputRenderer::finalizeReinit(const RenderTarget& render_target)
    {
        std::vector<AttachmentInfo> render_pass_attachments = reinitializeAttachments(render_target);
        createFrameBuffers(render_target, render_pass_attachments);
        _render_area.offset = { 0, 0 };
        _render_area.extent = render_target.getExtent();;
    }
}