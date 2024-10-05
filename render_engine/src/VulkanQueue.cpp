#include <render_engine/VulkanQueue.h>

#include <render_engine/synchronization/SyncOperations.h>

#include <render_engine/debug/Profiler.h>

#include <render_engine/window/SwapChain.h>


namespace RenderEngine
{
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
}