#include <render_engine/RenderEngine.h>

#include <render_engine/RenderContext.h>
#include <render_engine/RendererFactory.h>


namespace RenderEngine
{
	namespace
	{
		constexpr uint32_t kMaxNumOfResources = 10;
	}
	RenderEngine::RenderEngine(Device& device, VkQueue render_queue, uint32_t render_queue_family, size_t back_buffer_count)
		: _device(device)
		, _render_queue(render_queue)
		, _render_queue_family(render_queue_family)
		, _gpu_resource_manager(device.getPhysicalDevice(), device.getLogicalDevice(), back_buffer_count, kMaxNumOfResources)
		, _command_pool_factory(device.getLogicalDevice(), render_queue_family)
	{

	}

	void RenderEngine::submitDrawCalls(const std::vector<VkCommandBufferSubmitInfo>& command_buffers, const SynchronizationPrimitives& synchronization_primitives)
	{
		VkSubmitInfo2 submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

		submit_info.commandBufferInfoCount = command_buffers.size();
		submit_info.pCommandBufferInfos = command_buffers.data();


		submit_info.waitSemaphoreInfoCount = synchronization_primitives.wait_semaphores.size();
		submit_info.pWaitSemaphoreInfos = synchronization_primitives.wait_semaphores.data();

		submit_info.signalSemaphoreInfoCount = synchronization_primitives.signal_semaphores.size();
		submit_info.pSignalSemaphoreInfos = synchronization_primitives.signal_semaphores.data();

		if (vkQueueSubmit2(_render_queue, 1, &submit_info, synchronization_primitives.on_finished_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}

}