#include <render_engine/CommandContext.h>

#include <render_engine/synchronization/SyncOperations.h>

#include <algorithm>
#include <cassert>
#include <ranges>
#include <shared_mutex>
#include <stdexcept>

namespace RenderEngine
{

#pragma region AbstractCommandContext

    class AbstractCommandContext::QueueLoadBalancer
    {
    public:

        explicit QueueLoadBalancer(LogicalDevice& logical_device, uint32_t queue_family_index, uint32_t queue_count)
        {
            for (uint32_t i = 0; i < queue_count; ++i)
            {
                VkQueue queue;
                logical_device->vkGetDeviceQueue(*logical_device, queue_family_index, i, &queue);

                _queue_map.emplace_back(queue);
            }
        }
        GuardedQueue getQueue()
        {
            std::lock_guard lock{ _queue_map_mutex };
            auto it = std::min_element(_queue_map.begin(), _queue_map.end(),
                                       [](auto& a, auto& b) { return a.access_count < b.access_count; });
            it->access_count++;
            return GuardedQueue{ it->queue,  std::unique_lock{it->access_mutex} };
        }
    private:
        struct QueueData
        {
            VkQueue queue{ VK_NULL_HANDLE };
            std::shared_mutex access_mutex;
            uint32_t access_count{ 0 };

            explicit QueueData(VkQueue queue)
                : queue(queue)
            {}
            QueueData(const QueueData&) = delete;
            QueueData(QueueData&&) = delete;

            QueueData& operator=(const QueueData&) = delete;
            QueueData& operator=(QueueData&&) = delete;
        };
        std::list<QueueData> _queue_map;
        std::mutex _queue_map_mutex;
    };

    AbstractCommandContext::AbstractCommandContext(LogicalDevice& logical_device,
                                                   uint32_t queue_family_index,
                                                   DeviceLookup::QueueFamilyInfo queue_family_info)
        : _logical_device(&logical_device)
        , _queue_family_index(queue_family_index)
        , _queue_family_info(std::move(queue_family_info))
    {
        assert(_queue_family_info.hasComputeSupport != std::nullopt
               && _queue_family_info.hasGraphicsSupport != std::nullopt
               && _queue_family_info.hasTransferSupport != std::nullopt
               && _queue_family_info.queue_count != std::nullopt);
        _queue_load_balancer = std::make_unique<QueueLoadBalancer>(logical_device, _queue_family_index, *_queue_family_info.queue_count);
    }


    AbstractCommandContext::AbstractCommandContext(AbstractCommandContext&& o) noexcept
    {
        *this = std::move(o);
    }

    AbstractCommandContext& AbstractCommandContext::operator=(AbstractCommandContext&& o) noexcept
    {
        using std::swap;
        swap(o._logical_device, _logical_device);
        swap(o._queue_family_index, _queue_family_index);
        swap(o._queue_family_info, _queue_family_info);
        swap(o._queue_load_balancer, _queue_load_balancer);
        return *this;
    }

    void AbstractCommandContext::queueSubmit(VkSubmitInfo2&& submit_info,
                                             const SyncOperations& sync_operations,
                                             VkFence fence)
    {
        sync_operations.fillInfo(submit_info);

        auto queue = _queue_load_balancer->getQueue();
        if ((*_logical_device)->vkQueueSubmit2(queue.getQueue(),
                                               1,
                                               &submit_info,
                                               fence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

    void AbstractCommandContext::queuePresent(VkPresentInfoKHR&& present_info, const SyncOperations& sync_operations)
    {
        sync_operations.fillInfo(present_info);

        auto queue = _queue_load_balancer->getQueue();
        if ((*_logical_device)->vkQueuePresentKHR(queue.getQueue(), &present_info) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain");
        }
    }

    GuardedQueue AbstractCommandContext::getQueue()
    {
        return _queue_load_balancer->getQueue();
    }

    bool AbstractCommandContext::isPipelineStageSupported(VkPipelineStageFlags2 pipeline_stage) const
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

#pragma region SingleShotCommandContext

    class SingleShotCommandContext::ThreadData
    {
    public:
        ThreadData(LogicalDevice& logical_device, uint32_t queue_family_index)
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
        ~ThreadData()
        {
            if (_transient_command_pool != VK_NULL_HANDLE)
            {
                _logical_device->vkDestroyCommandPool(*_logical_device, _transient_command_pool, nullptr);
            }
        }

        ThreadData(const ThreadData&) = delete;
        ThreadData(ThreadData&&) = delete;

        ThreadData& operator=(const ThreadData&) = delete;
        ThreadData& operator=(ThreadData&&) = delete;

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

    SingleShotCommandContext::~SingleShotCommandContext() = default;

    SingleShotCommandContext::SingleShotCommandContext(LogicalDevice& logical_device,
                                                       uint32_t queue_family_index,
                                                       DeviceLookup::QueueFamilyInfo queue_family_info,
                                                       CreationToken)
        : AbstractCommandContext(logical_device, queue_family_index, std::move(queue_family_info))
    {

    }

    SingleShotCommandContext::SingleShotCommandContext(SingleShotCommandContext&& o) noexcept
        : AbstractCommandContext(std::move(o))
    {
        *this = std::move(o);
    }

    SingleShotCommandContext& SingleShotCommandContext::operator=(SingleShotCommandContext&& o) noexcept
    {
        std::scoped_lock lock{ o._thread_data_mutex, _thread_data_mutex };
        using std::swap;
        AbstractCommandContext::operator=(std::move(o));
        swap(o._thread_data, _thread_data);
        return *this;
    }

    VkCommandBuffer SingleShotCommandContext::createCommandBuffer()
    {
        std::lock_guard lock{ _thread_data_mutex };
        auto thread_id = std::this_thread::get_id();
        auto it = _thread_data.find(thread_id);
        if (it == _thread_data.end())
        {
            it = _thread_data.insert({ thread_id, std::make_unique<ThreadData>(getLogicalDevice(), getQueueFamilyIndex()) }).first;
        }
        return it->second->createSingleShotCommandBuffer();
    }

#pragma endregion

#pragma region CommandContext

    class CommandContext::ThreadData
    {
    public:
        ThreadData(LogicalDevice& logical_device, uint32_t back_buffer_size, uint32_t queue_family_index)
            : _logical_device(logical_device)
        {
            for (uint32_t i = 0; i < back_buffer_size; ++i)
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
                _command_pools_per_frame.emplace_back(pool);
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
        ~ThreadData()
        {
            for (VkCommandPool pool : _command_pools_per_frame)
            {
                _logical_device->vkDestroyCommandPool(*_logical_device, pool, nullptr);
            }
            if (_transient_command_pool != VK_NULL_HANDLE)
            {
                _logical_device->vkDestroyCommandPool(*_logical_device, _transient_command_pool, nullptr);
            }
        }

        ThreadData(const ThreadData&) = delete;
        ThreadData(ThreadData&&) = delete;

        ThreadData& operator=(const ThreadData&) = delete;
        ThreadData& operator=(ThreadData&&) = delete;

        VkCommandPool getCommandPool(uint32_t render_target_image_index)
        {
            return _command_pools_per_frame[render_target_image_index];
        }
        std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count, uint32_t render_target_image_index)
        {
            std::vector<VkCommandBuffer> command_buffers(count, VK_NULL_HANDLE);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = getCommandPool(render_target_image_index);
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
        std::vector<VkCommandPool> _command_pools_per_frame;
        VkCommandPool _transient_command_pool{ VK_NULL_HANDLE };

    };

    CommandContext::CommandContext(LogicalDevice& logical_device,
                                   uint32_t queue_family_index,
                                   DeviceLookup::QueueFamilyInfo queue_family_info,
                                   uint32_t back_buffer_size,
                                   CreationToken)
        : AbstractCommandContext(logical_device, queue_family_index, std::move(queue_family_info))
        , _back_buffer_size(back_buffer_size)
    {}
    CommandContext::CommandContext(CommandContext&& o) noexcept
        : AbstractCommandContext(std::move(o))
    {
        *this = std::move(o);
    }
    CommandContext& CommandContext::operator=(CommandContext&& o) noexcept
    {
        std::scoped_lock lock{ o._thread_data_mutex, _thread_data_mutex };
        using std::swap;
        AbstractCommandContext::operator=(std::move(o));
        swap(o._thread_data, _thread_data);
        swap(o._back_buffer_size, _back_buffer_size);
        return *this;
    }
    CommandContext::~CommandContext() = default;


    VkCommandBuffer CommandContext::createCommandBuffer(uint32_t render_target_image_index)
    {
        return createCommandBuffers(1, render_target_image_index).front();
    }

    std::vector<VkCommandBuffer> CommandContext::createCommandBuffers(uint32_t count, uint32_t render_target_image_index)
    {
        std::lock_guard lock{ _thread_data_mutex };
        auto thread_id = std::this_thread::get_id();
        auto it = _thread_data.find(thread_id);
        if (it == _thread_data.end())
        {
            it = _thread_data.insert({ thread_id, std::make_unique<ThreadData>(getLogicalDevice(),
                                                                               _back_buffer_size,
                                                                               getQueueFamilyIndex()) }).first;
        }
        return it->second->createCommandBuffers(count, render_target_image_index);
    }

    std::shared_ptr<CommandContext> CommandContext::clone() const
    {
        return std::make_shared<CommandContext>(getLogicalDevice(),
                                                getQueueFamilyIndex(),
                                                getQueueFamilyInfo(),
                                                _back_buffer_size,
                                                CommandContext::CreationToken{});
    }
#pragma endregion
}