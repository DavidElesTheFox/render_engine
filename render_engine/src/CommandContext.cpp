#include <render_engine/CommandContext.h>

#include <algorithm>
#include <cassert>
#include <ranges>
#include <stdexcept>

namespace RenderEngine
{
    CommandContext::CommandContext(VkDevice logical_device,
                                   uint32_t queue_family_index,
                                   DeviceLookup::QueueFamilyInfo queue_family_info,
                                   CreationToken)
        : _logical_device(logical_device)
        , _queue_family_index(queue_family_index)
        , _queue_family_info(std::move(queue_family_info))
    {
        assert(logical_device != VK_NULL_HANDLE);
        assert(_queue_family_info.hasComputeSupport != std::nullopt
               && _queue_family_info.hasGraphicsSupport != std::nullopt
               && _queue_family_info.hasTransferSupport != std::nullopt);

        vkGetDeviceQueue(_logical_device, queue_family_index, 0, &_queue);
        {
            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            pool_info.queueFamilyIndex = queue_family_index;
            if (vkCreateCommandPool(_logical_device, &pool_info, nullptr, &_command_pool) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create command pool!");
            }
        }
        {
            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            pool_info.queueFamilyIndex = queue_family_index;
            if (vkCreateCommandPool(_logical_device, &pool_info, nullptr, &_transient_command_pool) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create command pool!");
            }
        }
    }
    CommandContext::CommandContext(CommandContext&& o)
    {
        *this = std::move(o);
    }
    CommandContext& CommandContext::operator=(CommandContext&& o)
    {
        using std::swap;
        swap(o._logical_device, _logical_device);
        swap(o._queue_family_index, _queue_family_index);
        swap(o._queue, _queue);
        swap(o._command_pool, _command_pool);
        swap(o._transient_command_pool, _transient_command_pool);
        return *this;
    }
    CommandContext::~CommandContext()
    {
        if (_logical_device == VK_NULL_HANDLE)
        {
            return;
        }
        vkDestroyCommandPool(_logical_device, _command_pool, nullptr);
        vkDestroyCommandPool(_logical_device, _transient_command_pool, nullptr);
    }
    VkQueue CommandContext::getQueue() const
    {
        return _queue;
    }
    VkCommandBuffer CommandContext::createCommandBuffer(Usage usage) const
    {
        return createCommandBuffers(1, usage).front();
    }
    std::vector<VkCommandBuffer> CommandContext::createCommandBuffers(uint32_t count, Usage usage) const
    {
        std::vector<VkCommandBuffer> command_buffers(count, VK_NULL_HANDLE);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = getCommandPool(usage);
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = command_buffers.size();

        if (vkAllocateCommandBuffers(_logical_device, &allocInfo, command_buffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        return command_buffers;
    }

    std::shared_ptr<CommandContext> CommandContext::clone() const
    {
        return std::make_shared<CommandContext>(_logical_device, _queue_family_index, _queue_family_info, CommandContext::CreationToken{});
    }

    bool CommandContext::isPipelineStageSupported(VkPipelineStageFlags2 pipeline_stage)
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

}