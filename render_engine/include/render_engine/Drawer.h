#pragma once
#include <render_engine/SwapChain.h>

#include <vulkan/vulkan.h>

#include <vector>
namespace RenderEngine
{
	class Drawer
	{
	public:
		Drawer(VkDevice logical_device,
			VkRenderPass render_pass,
			VkPipeline pipeline,
			VkPipelineLayout pipeline_layout,
			const SwapChain& swap_chain,
			uint32_t render_queue_family);
		void draw(uint32_t back_buffer_index)
		{
			draw(_frame_buffers[back_buffer_index]);
		}
		std::vector<VkCommandBuffer> getCommandBuffers()
		{
			return { _command_buffer };
		}
		~Drawer();
	private:
		void createFrameBuffers(const SwapChain& swap_chain);
		void createCommandPool(uint32_t render_queue_family);
		void createCommandBuffer();
		void draw(const VkFramebuffer& frame_buffer);

		VkDevice _logical_device;
		VkRenderPass _render_pass;
		VkPipelineLayout _pipeline_layout;
		VkPipeline _pipeline;
		std::vector<VkFramebuffer> _frame_buffers;
		VkCommandPool _command_pool;
		VkCommandBuffer _command_buffer;
		VkRect2D _render_area;
	};
}