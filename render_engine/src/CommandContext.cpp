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

#pragma region VulkanQueue


    VulkanQueue::VulkanQueue(LogicalDevice* logical_device,
                             uint32_t queue_family_index,
                             uint32_t queue_count,
                             DeviceLookup::QueueFamilyInfo queue_family_info)
        : _logical_device(logical_device)
        , _queue_family_index(queue_family_index)
        , _queue_family_info(std::move(queue_family_info))
        , _queue_load_balancer(*logical_device, queue_family_index, queue_count)
    {
        if (queue_family_info.queue_count != std::nullopt
            && queue_family_info.queue_count < queue_count)
        {
            throw std::runtime_error(std::format("Requested queue count ({:d}) is bigger then the available ({:d})", queue_count, *queue_family_info.queue_count));
        }
    }

    void VulkanQueue::queueSubmit(VkSubmitInfo2&& submit_info,
                                  const SyncOperations& sync_operations,
                                  VkFence fence)
    {
        PROFILE_SCOPE();
        sync_operations.fillInfo(submit_info);

        auto queue = _queue_load_balancer.getQueue();
        if ((*_logical_device)->vkQueueSubmit2(queue.access(),
                                               1,
                                               &submit_info,
                                               fence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

    void VulkanQueue::queuePresent(VkPresentInfoKHR&& present_info,
                                   const SyncOperations& sync_operations,
                                   SwapChain& swap_chain)
    {
        PROFILE_SCOPE();

        sync_operations.fillInfo(present_info);

        auto queue = _queue_load_balancer.getQueue();
        swap_chain.present(queue.access(), std::move(present_info));
    }

    GuardedQueue VulkanQueue::getVulkanGuardedQueue()
    {
        return _queue_load_balancer.getQueue();
    }

    bool VulkanQueue::isPipelineStageSupported(VkPipelineStageFlags2 pipeline_stage) const
    {
        switch (pipeline_stage)
        {
            case VK_PIPELINE_STAGE_2_NONE:
            case VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT:
            case VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT:
            case VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT:
            case VK_PIPELINE_STAGE_2_HOST_BIT:
                return true;
            case VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT:
            case VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT:
            case VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT:
            case VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT:
            case VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT:
            case VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT:
            case VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT:
            case VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT:
            case VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT:
            case VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT:
            case VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT:
            case VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT:
            case VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT:
            case VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT:
            case VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT:
            case VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT:
            case VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR:
            case VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR:
            case VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR:
            case VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT:
            case VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV:
            case VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV:
            case VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR:
                return *_queue_family_info.hasGraphicsSupport;
            case VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT:
                return *_queue_family_info.hasComputeSupport;
            case VK_PIPELINE_STAGE_2_TRANSFER_BIT:
            case VK_PIPELINE_STAGE_2_COPY_BIT:
            case VK_PIPELINE_STAGE_2_RESOLVE_BIT:
            case VK_PIPELINE_STAGE_2_BLIT_BIT:
            case VK_PIPELINE_STAGE_2_CLEAR_BIT:
                return *_queue_family_info.hasTransferSupport;
            default:
                assert("Unspecified pipeline stage");
                return false;
        }
    }

#pragma endregion

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