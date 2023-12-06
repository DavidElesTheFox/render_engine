#include <render_engine/renderers/SingleColorOutputRenderer.h>

#include <render_engine/resources/RenderTarget.h>
#include <render_engine/window/Window.h>

namespace RenderEngine
{
    SingleColorOutputRenderer::SingleColorOutputRenderer(IWindow& parent)
        : _window(parent)
    {

    }
    SingleColorOutputRenderer::~SingleColorOutputRenderer()
    {
        destroyRenderOutput();
    }
    void SingleColorOutputRenderer::initializeRendererOutput(const RenderTarget& render_target,
                                                             VkRenderPass render_pass,
                                                             size_t back_buffer_size)
    {
        _render_pass = render_pass;
        _back_buffer.resize(back_buffer_size);

        _render_area.offset = { 0, 0 };
        _render_area.extent = render_target.getExtent();
        createFrameBuffers(render_target);
        _command_pool = _window.getRenderEngine().getCommandPoolFactory().getCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        createCommandBuffer();

    }
    void SingleColorOutputRenderer::destroyRenderOutput()
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        vkDestroyCommandPool(logical_device, _command_pool.command_pool, nullptr);

        resetFrameBuffers();

        vkDestroyRenderPass(logical_device, _render_pass, nullptr);
    }
    void SingleColorOutputRenderer::createFrameBuffers(const RenderTarget& render_target)
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        _frame_buffers.resize(render_target.getImageViews().size());
        for (uint32_t i = 0; i < _frame_buffers.size(); ++i)
        {
            if (createFrameBuffer(render_target, i) == false)
            {
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroyFramebuffer(logical_device, _frame_buffers[j], nullptr);
                }
                _frame_buffers.clear();
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    bool SingleColorOutputRenderer::createFrameBuffer(const RenderTarget& render_target, uint32_t frame_buffer_index)
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        VkImageView attachments[] = {
                render_target.getImageViews()[frame_buffer_index]
        };
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = render_target.getWidth();
        framebuffer_info.height = render_target.getHeight();
        framebuffer_info.layers = 1;

        return vkCreateFramebuffer(logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) == VK_SUCCESS;
    }
    void SingleColorOutputRenderer::createCommandBuffer()
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        for (FrameData& frame_data : _back_buffer)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = _command_pool.command_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(logical_device, &allocInfo, &frame_data.command_buffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }
    }
    void SingleColorOutputRenderer::resetFrameBuffers()
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        for (auto framebuffer : _frame_buffers)
        {
            vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
        }
    }
    void SingleColorOutputRenderer::beforeReinit()
    {
        resetFrameBuffers();
    }
    void SingleColorOutputRenderer::finalizeReinit(const RenderTarget& render_target)
    {
        createFrameBuffers(render_target);
        _render_area.offset = { 0, 0 };
        _render_area.extent = render_target.getExtent();;
    }
}