#include <render_engine/CommandContext.h>

#include <render_engine/debug/Profiler.h>
#include <render_engine/synchronization/SyncOperations.h>

#include <algorithm>
#include <cassert>
#include <ranges>
#include <shared_mutex>
#include <stdexcept>

namespace RenderEngine
{

#pragma region SingleShotCommandBufferFactory

    class SingleShotCommandBufferFactory::Tray
    {
    public:
        Tray(LogicalDevice& logical_device, uint32_t queue_family_index)
            : _logical_device(logical_device)
        {
            {
                VkCommandPoolCreateInfo pool_info{};
                pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                pool_info.queueFamilyIndex = queue_family_index;
                if (logical_device->vkCreateCommandPool(*logical_device, &pool_info, nullptr, &_transient_command_pool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create command pool!");
                }
            }
        }
        ~Tray()
        {
            if (_transient_command_pool != VK_NULL_HANDLE)
            {
                _logical_device->vkDestroyCommandPool(*_logical_device, _transient_command_pool, nullptr);
            }
        }

        Tray(const Tray&) = delete;
        Tray(Tray&&) = delete;

        Tray& operator=(const Tray&) = delete;
        Tray& operator=(Tray&&) = delete;

        VkCommandBuffer createSingleShotCommandBuffer()
        {
            std::vector<VkCommandBuffer> command_buffers(1, VK_NULL_HANDLE);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = _transient_command_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

            if (_logical_device->vkAllocateCommandBuffers(*_logical_device, &allocInfo, command_buffers.data()) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }

            return command_buffers.front();
        }
    private:
        LogicalDevice& _logical_device;
        VkCommandPool _transient_command_pool{ VK_NULL_HANDLE };

    };

    SingleShotCommandBufferFactory::~SingleShotCommandBufferFactory() = default;

    SingleShotCommandBufferFactory::SingleShotCommandBufferFactory(RefObj<VulkanQueue> queue,
                                                                   CreationToken)
        : _vulkan_queue(std::move(queue))
    {

    }



    VkCommandBuffer SingleShotCommandBufferFactory::createCommandBuffer(uint32_t tray_index)
    {
        std::lock_guard lock{ _trays_mutex };
        auto it = _trays.find(tray_index);
        if (it == _trays.end())
        {
            it = _trays.insert({ tray_index, std::make_unique<Tray>(_vulkan_queue->getLogicalDevice(), _vulkan_queue->getQueueFamilyIndex()) }).first;
        }
        return it->second->createSingleShotCommandBuffer();
    }

#pragma endregion

#pragma region CommandBufferFactory

    class CommandBufferFactory::Tray
    {
    public:
        Tray(LogicalDevice& logical_device, uint32_t num_of_pools, uint32_t queue_family_index)
            : _logical_device(logical_device)
        {
            for (uint32_t i = 0; i < num_of_pools; ++i)
            {
                VkCommandPool pool{ VK_NULL_HANDLE };
                VkCommandPoolCreateInfo pool_info{};
                pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                pool_info.queueFamilyIndex = queue_family_index;
                if (logical_device->vkCreateCommandPool(*logical_device, &pool_info, nullptr, &pool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create command pool!");
                }
                _command_pools.emplace_back(pool);
            }
            {
                VkCommandPoolCreateInfo pool_info{};
                pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                pool_info.queueFamilyIndex = queue_family_index;
                if (logical_device->vkCreateCommandPool(*logical_device, &pool_info, nullptr, &_transient_command_pool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create command pool!");
                }
            }
        }
        ~Tray()
        {
            for (VkCommandPool pool : _command_pools)
            {
                _logical_device->vkDestroyCommandPool(*_logical_device, pool, nullptr);
            }
            if (_transient_command_pool != VK_NULL_HANDLE)
            {
                _logical_device->vkDestroyCommandPool(*_logical_device, _transient_command_pool, nullptr);
            }
        }

        Tray(const Tray&) = delete;
        Tray(Tray&&) = delete;

        Tray& operator=(const Tray&) = delete;
        Tray& operator=(Tray&&) = delete;

        VkCommandPool getCommandPool(uint32_t pool_index)
        {
            return _command_pools[pool_index];
        }
        std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count, uint32_t pool_index)
        {
            std::vector<VkCommandBuffer> command_buffers(count, VK_NULL_HANDLE);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = getCommandPool(pool_index);
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

            if (_logical_device->vkAllocateCommandBuffers(*_logical_device, &allocInfo, command_buffers.data()) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }

            return command_buffers;
        }

        VkCommandBuffer createSingleShotCommandBuffer()
        {
            std::vector<VkCommandBuffer> command_buffers(1, VK_NULL_HANDLE);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = _transient_command_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

            if (_logical_device->vkAllocateCommandBuffers(*_logical_device, &allocInfo, command_buffers.data()) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }

            return command_buffers.front();
        }
    private:
        LogicalDevice& _logical_device;
        std::vector<VkCommandPool> _command_pools;
        VkCommandPool _transient_command_pool{ VK_NULL_HANDLE };

    };

    CommandBufferFactory::CommandBufferFactory(RefObj<VulkanQueue> vulkan_queue,
                                               uint32_t num_of_pools_per_tray,
                                               CreationToken)
        : _vulkan_queue(std::move(vulkan_queue))
        , _num_of_pools_per_tray(num_of_pools_per_tray)
    {}
    CommandBufferFactory::~CommandBufferFactory() = default;


    VkCommandBuffer CommandBufferFactory::createCommandBuffer(uint32_t tray_index, uint32_t pool_index)
    {
        return createCommandBuffers(1, tray_index, pool_index).front();
    }

    std::vector<VkCommandBuffer> CommandBufferFactory::createCommandBuffers(uint32_t count, uint32_t tray_index, uint32_t pool_index)
    {
        std::lock_guard lock{ _trays_mutex };
        auto it = _trays.find(tray_index);
        if (it == _trays.end())
        {
            it = _trays.insert({ tray_index, std::make_unique<Tray>(_vulkan_queue->getLogicalDevice(),
                                                                    _num_of_pools_per_tray,
                                                                    _vulkan_queue->getQueueFamilyIndex()) }).first;
        }
        return it->second->createCommandBuffers(count, pool_index);
    }

#pragma endregion
}