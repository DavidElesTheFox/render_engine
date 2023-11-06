#pragma once

#include <volk.h>

#include <vector>
#include <stdexcept>
namespace RenderEngine
{
	struct SynchronizationPrimitives
	{
		std::vector<VkSemaphoreSubmitInfo> wait_semaphores;
		std::vector<VkSemaphoreSubmitInfo> signal_semaphores;
		VkFence on_finished_fence{ VK_NULL_HANDLE };

		static SynchronizationPrimitives CreateWithFence(VkDevice logical_device)
		{
			SynchronizationPrimitives synchronization_primitives;
			VkFenceCreateInfo fence_info{};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateFence(logical_device, &fence_info, nullptr, &synchronization_primitives.on_finished_fence) != VK_SUCCESS)
			{
				throw std::runtime_error("Cannot create fence for upload");
			}
			vkResetFences(logical_device, 1, &synchronization_primitives.on_finished_fence);
			return synchronization_primitives;
		}
	};
}