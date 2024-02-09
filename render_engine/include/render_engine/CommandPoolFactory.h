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

        CommandPoolFactory(VkDevice logical_device, uint32_t queue_family)
            : _logical_device(logical_device)
            , _queue_family(queue_family)
        {}

        CommandPool getCommandPool(VkCommandPoolCreateFlags flags);
        CommandPool getCommandPool(VkCommandPoolCreateFlags flags, uint32_t queue_family_override);
    private:
        VkDevice _logical_device{ VK_NULL_HANDLE };
        uint32_t _queue_family{ 0 };

    };
}