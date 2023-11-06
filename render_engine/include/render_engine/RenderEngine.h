#pragma once

#include <volk.h>

#include <render_engine/GpuResourceManager.h>
#include <render_engine/Device.h>

#include <unordered_map>

#include <ranges>
#include <algorithm>

namespace RenderEngine
{
	class AbstractRenderer;

	class RenderEngine
	{
	public:
		struct SynchronizationPrimitives
		{
			std::vector<VkSemaphoreSubmitInfo> wait_semaphores;
			std::vector<VkSemaphoreSubmitInfo> signal_semaphores;
			VkFence on_finished_fence{ VK_NULL_HANDLE };
		};
		RenderEngine(Device& device, VkQueue _render_queue, uint32_t _render_queue_family, size_t back_buffer_count);


		uint32_t getRenderQueueFamily() { return _render_queue_family; }
		VkQueue& getRenderQueue() { return _render_queue; }

		void render(const SynchronizationPrimitives& synchronization_primitives,
			const std::ranges::input_range auto& renderers, 
			uint32_t image_index,
			uint32_t frame_id)
		{
			std::vector<VkCommandBufferSubmitInfo> command_buffer_infos = collectCommandBuffers(renderers, image_index, frame_id);
			submitDrawCalls(command_buffer_infos, synchronization_primitives);
		}

		GpuResourceManager& getGpuResourceManager() { return _gpu_resource_manager; }
	private:
		std::vector<VkCommandBufferSubmitInfo> collectCommandBuffers(const std::ranges::input_range auto& renderers,
			uint32_t image_index,
			uint32_t frame_id)
		{
			std::vector<VkCommandBufferSubmitInfo> command_buffers;
			for (AbstractRenderer* drawer : renderers)
			{
				drawer->draw(image_index, frame_id);
				auto current_command_buffers = drawer->getCommandBuffers(frame_id);
				std::ranges::transform(current_command_buffers,
					std::back_inserter(command_buffers),
					[](const auto& command_buffer) { return VkCommandBufferSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
						.pNext = VK_NULL_HANDLE,
						.commandBuffer = command_buffer,
						.deviceMask = 0 };
					});
			}
			return command_buffers;
		}

		void submitDrawCalls(const std::vector<VkCommandBufferSubmitInfo>& command_buffers,
			const RenderEngine::RenderEngine::SynchronizationPrimitives& synchronization_primitives);

		Device& _device;
		VkQueue _render_queue;
		uint32_t _render_queue_family{ 0 };
		GpuResourceManager _gpu_resource_manager;
	};
}