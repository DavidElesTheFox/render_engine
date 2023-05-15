#include <render_engine/Drawer.h>

#include <stdexcept>

namespace RenderEngine
{
	Drawer::Drawer(VkDevice logical_device,
		VkRenderPass render_pass,
		VkPipeline pipeline,
		VkPipelineLayout pipeline_layout,
		const SwapChain& swap_chain,
		uint32_t render_queue_family,
		uint32_t back_buffer_size)
		try : _logical_device(logical_device)
		, _render_pass(render_pass)
		, _pipeline_layout(pipeline_layout)
		, _pipeline(pipeline)
	{
		_back_buffer.resize(back_buffer_size);
		_render_area.offset = { 0, 0 };
		_render_area.extent = swap_chain.getDetails().extent;
		createFrameBuffers(swap_chain);
		createCommandPool(render_queue_family);
		createCommandBuffer();
	}
	catch (const std::runtime_error&)
	{
		vkDestroyCommandPool(_logical_device, _command_pool, nullptr);

		for (auto framebuffer : _frame_buffers) {
			vkDestroyFramebuffer(_logical_device, framebuffer, nullptr);
		}

		vkDestroyPipelineLayout(_logical_device, _pipeline_layout, nullptr);
		vkDestroyRenderPass(_logical_device, _render_pass, nullptr);
		vkDestroyPipeline(_logical_device, _pipeline, nullptr);
		throw;
	}

	void Drawer::draw(const VkFramebuffer& frame_buffer, uint32_t frame_number)
	{
		FrameData& frame_data = getFrameData(frame_number);
		vkResetCommandBuffer(frame_data.command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _render_pass;
		render_pass_info.framebuffer = frame_buffer;
		render_pass_info.renderArea = _render_area;

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clearColor;

		vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)_render_area.extent.width;
		viewport.height = (float)_render_area.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(frame_data.command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = _render_area.extent;
		vkCmdSetScissor(frame_data.command_buffer, 0, 1, &scissor);

		vkCmdDraw(frame_data.command_buffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(frame_data.command_buffer);

		if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	Drawer::ReinitializationCommand Drawer::reinit()
	{
		return ReinitializationCommand(*this);
	}

	Drawer::~Drawer()
	{
		vkDestroyCommandPool(_logical_device, _command_pool, nullptr);

		resetFrameBuffers();

		vkDestroyPipelineLayout(_logical_device, _pipeline_layout, nullptr);
		vkDestroyRenderPass(_logical_device, _render_pass, nullptr);
		vkDestroyPipeline(_logical_device, _pipeline, nullptr);
	}
	void Drawer::resetFrameBuffers()
	{
		for (auto framebuffer : _frame_buffers) {
			vkDestroyFramebuffer(_logical_device, framebuffer, nullptr);
		}
	}
	void Drawer::createFrameBuffers(const SwapChain& swap_chain)
	{
		_frame_buffers.resize(swap_chain.getDetails().image_views.size());
		for (uint32_t i = 0; i < swap_chain.getDetails().image_views.size(); ++i)
		{
			if (createFrameBuffer(swap_chain, i) == false)
			{
				for (uint32_t j = 0; j < i; ++j) {
					vkDestroyFramebuffer(_logical_device, _frame_buffers[j], nullptr);
				}
				_frame_buffers.clear();
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}
	bool Drawer::createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index)
	{
		VkImageView attachments[] = {
				swap_chain.getDetails().image_views[frame_buffer_index]
		};
		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = _render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = swap_chain.getDetails().extent.width;
		framebuffer_info.height = swap_chain.getDetails().extent.height;
		framebuffer_info.layers = 1;

		return vkCreateFramebuffer(_logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) == VK_SUCCESS;
	}
	void Drawer::createCommandPool(uint32_t render_queue_family)
	{
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = render_queue_family;
		if (vkCreateCommandPool(_logical_device, &pool_info, nullptr, &_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	void Drawer::createCommandBuffer()
	{
		for (FrameData& frame_data : _back_buffer)
		{
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = _command_pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			if (vkAllocateCommandBuffers(_logical_device, &allocInfo, &frame_data.command_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}
		}
	}
}