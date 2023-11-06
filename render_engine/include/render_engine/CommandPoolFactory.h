#pragma once

#include <volk.h>

#include <render_engine/Device.h>
namespace RenderEngine
{
	class CommandPoolFactory
	{
	public:
		struct CommandPool
		{
			VkCommandPool command_pool{ VK_NULL_HANDLE };
		};

		CommandPoolFactory(Device& device, uint32_t queue_family)
			: _device(device)
			, _queue_family(queue_family)
		{}

		CommandPool getCommandPool(VkCommandPoolCreateFlags flags);
	private:
		Device& _device;
		uint32_t _queue_family{ 0 };
		
	};
}