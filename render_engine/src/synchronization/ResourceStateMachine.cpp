#include <render_engine/synchronization/ResourceStateMachine.h>

#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

#include <cassert>
#include <stdexcept>
namespace RenderEngine
{
    void ResourceStateMachine::resetStages(Texture& texture)
    {
        texture.overrideResourceState(texture.getResourceState().clone()
                                      .setPipelineStage(VK_PIPELINE_STAGE_2_NONE)
                                      .setAccessFlag(VK_ACCESS_2_NONE));
    }

    void ResourceStateMachine::resetStages(Buffer& texture)
    {
        texture.overrideResourceState(texture.getResourceState().clone()
                                      .setPipelineStage(VK_PIPELINE_STAGE_2_NONE)
                                      .setAccessFlag(VK_ACCESS_2_NONE));
    }
    void ResourceStateMachine::recordStateChange(Texture* image, TextureState next_state)
    {
        if (next_state.layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            assert(image->getResourceState().layout != VK_IMAGE_LAYOUT_UNDEFINED);
            next_state.layout = image->getResourceState().layout;
        }
        _images[image] = std::move(next_state);
    }

    void ResourceStateMachine::recordStateChange(Buffer* buffer, BufferState next_state)
    {
        _buffers[buffer] = std::move(next_state);
    }

    void ResourceStateMachine::commitChanges(VkCommandBuffer command_buffer)
    {
        auto image_barriers = createImageBarriers();
        auto buffer_barriers = createBufferBarriers();
        if (image_barriers.empty() && buffer_barriers.empty())
        {
            return;
        }
        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = image_barriers.size();
        dependency.pImageMemoryBarriers = image_barriers.data();
        dependency.bufferMemoryBarrierCount = buffer_barriers.size();
        dependency.pBufferMemoryBarriers = buffer_barriers.data();
        vkCmdPipelineBarrier2(command_buffer, &dependency);
    }

    std::vector<VkImageMemoryBarrier2> ResourceStateMachine::createImageBarriers()
    {
        std::vector<VkImageMemoryBarrier2> image_barriers;
        for (auto& [texture, next_state] : _images)
        {
            auto state_description = texture->getResourceState();

            /* When the current state can't make any changes on the memory doesn't make any sense to
            make the memory available at that stage. Thus, keep the current state but change the barriare definition
            accordingly.
             */
            if (stateCanMakeChangesOnMemory(state_description.access_flag) == false)
            {
                state_description.access_flag = VK_ACCESS_2_NONE_KHR;
                state_description.pipeline_stage = VK_PIPELINE_STAGE_2_NONE;
            }

            if (next_state == std::nullopt
                || next_state == state_description)
            {
                continue;
            }
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = texture->getVkImage();
            barrier.srcStageMask = state_description.pipeline_stage;
            barrier.srcAccessMask = state_description.access_flag;
            barrier.dstStageMask = next_state->pipeline_stage;
            barrier.dstAccessMask = next_state->access_flag;

            if (state_description.queue_family_index || next_state->queue_family_index)
            {
                assert(state_description.queue_family_index && next_state->queue_family_index
                       && "If one of the state has a queue family index then both needs to have");
                if (state_description.queue_family_index != next_state->queue_family_index)
                {
                    barrier.srcQueueFamilyIndex = *state_description.queue_family_index;
                    barrier.dstQueueFamilyIndex = *next_state->queue_family_index;
                }
            }

            barrier.oldLayout = state_description.layout;
            barrier.newLayout = next_state->layout;
            barrier.subresourceRange = texture->createSubresourceRange();
            image_barriers.emplace_back(std::move(barrier));
            texture->overrideResourceState(*next_state);
        }
        _images.clear();
        return image_barriers;
    }

    std::vector<VkBufferMemoryBarrier2> ResourceStateMachine::createBufferBarriers()
    {
        std::vector<VkBufferMemoryBarrier2> buffer_barriers;
        for (auto& [buffer, next_state] : _buffers)
        {
            auto state_description = buffer->getResourceState();
            /* When the current state can't make any changes on the memory doesn't make any sense to
            make the memory available at that stage. Thus, keep the current state but change the barriare definition
            accordingly.
             */
            if (stateCanMakeChangesOnMemory(state_description.access_flag) == false)
            {
                state_description.access_flag = VK_ACCESS_2_NONE_KHR;
                state_description.pipeline_stage = VK_PIPELINE_STAGE_2_NONE;
            }
            if (next_state == std::nullopt
                || next_state == state_description)
            {
                continue;
            }
            VkBufferMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.buffer = buffer->getBuffer();

            barrier.srcStageMask = state_description.pipeline_stage;
            barrier.srcAccessMask = state_description.access_flag;

            barrier.dstStageMask = next_state->pipeline_stage;
            barrier.dstAccessMask = next_state->access_flag;

            if (state_description.queue_family_index || next_state->queue_family_index)
            {
                assert(state_description.queue_family_index && next_state->queue_family_index
                       && "If one of the state has a queue family index then both needs to have");
                if (state_description.queue_family_index != next_state->queue_family_index)
                {
                    barrier.srcQueueFamilyIndex = *state_description.queue_family_index;
                    barrier.dstQueueFamilyIndex = *next_state->queue_family_index;
                }
            }
            barrier.offset = 0;
            barrier.size = buffer->getDeviceSize();
            buffer_barriers.emplace_back(std::move(barrier));
            buffer->overrideResourceState(*next_state);
        }
        _buffers.clear();
        return buffer_barriers;
    }

    bool ResourceStateMachine::stateCanMakeChangesOnMemory(VkAccessFlags2 access)
    {
        switch (access)
        {
            case VK_ACCESS_2_SHADER_WRITE_BIT:
            case VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT:
            case VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
            case VK_ACCESS_2_TRANSFER_WRITE_BIT:
            case VK_ACCESS_2_HOST_WRITE_BIT:
            case VK_ACCESS_2_MEMORY_WRITE_BIT:
            case VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT:
            case VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT:
            case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT:
            case VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR:
            case VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT:
            case VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV:
                return true;
            default:
                return false;
                break;
        }
    }


}