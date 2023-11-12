#include <render_engine/CommandPoolFactory.h>

namespace RenderEngine
{
	CommandPoolFactory::CommandPool CommandPoolFactory::getCommandPool(VkCommandPoolCreateFlags flags)
	{
		CommandPool result;

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = flags;
		pool_info.queueFamilyIndex = _queue_family;
		if (vkCreateCommandPool(_logical_device, &pool_info, nullptr, &result.command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
		return result;
	}
}