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
			uint32_t render_queue_family,
			uint32_t back_buffer_size);
		void draw(uint32_t swap_chain_image_index, uint32_t frame_number)
		{
			draw(_frame_buffers[swap_chain_image_index], frame_number);
		}
		std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number)
		{
			return { getFrameData(frame_number).command_buffer };
		}
		~Drawer();
	private:
		struct FrameData
		{
			VkCommandBuffer command_buffer;
		};
		void createFrameBuffers(const SwapChain& swap_chain);
		void createCommandPool(uint32_t render_queue_family);
		void createCommandBuffer();
		FrameData& getFrameData(uint32_t frame_number)
		{
			return _back_buffer[frame_number % _back_buffer.size()];
		}
		void draw(const VkFramebuffer& frame_buffer, uint32_t frame_number);

		VkDevice _logical_device;
		VkRenderPass _render_pass;
		VkPipelineLayout _pipeline_layout;
		VkPipeline _pipeline;
		std::vector<VkFramebuffer> _frame_buffers;
		VkCommandPool _command_pool;
		std::vector<FrameData> _back_buffer;
		VkRect2D _render_area;
	};
}