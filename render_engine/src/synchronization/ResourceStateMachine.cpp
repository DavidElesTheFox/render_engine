#include <render_engine/synchronization/ResourceStateMachine.h>

#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

#include <cassert>
#include <stdexcept>
namespace RenderEngine
{
    namespace
    {

    }
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

    SyncObject ResourceStateMachine::transferOwnership(Texture* texture,
                                                       TextureState new_state,
                                                       CommandContext* src,
                                                       CommandContext* dst,
                                                       const SyncOperations& sync_operations)
    {
        auto src_command_buffer = src->createCommandBuffer(CommandContext::Usage::SingleSubmit);
        auto dst_command_buffer = dst->createCommandBuffer(CommandContext::Usage::SingleSubmit);

        assert(src->getLogicalDevice() == dst->getLogicalDevice());
        assert(dst->isPipelineStageSupported(new_state.pipeline_stage));
        SyncObject sync_object = SyncObject::CreateWithFence(src->getLogicalDevice(), 0);
        sync_object.createTimelineSemaphore("TransferFinished", 0, 2);
        // sync_object.addSignalOperationToGroup(SyncGroups::kInternal, "TransferFinished", VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 1);
         //sync_object.addWaitOperationToGroup(SyncGroups::kExternal, "TransferFinished", VK_PIPELINE_STAGE_2_NONE, 1);

        ResourceStateMachine src_state_machine;
        ResourceStateMachine dst_state_machine;
        src_state_machine._apply_barrier_even_without_state_change = true;
        dst_state_machine._apply_barrier_even_without_state_change = true;

        const std::string texture_semaphore_name = std::format("{:#x}", reinterpret_cast<intptr_t>(texture));
        sync_object.createTimelineSemaphore(texture_semaphore_name, 0, 2);

        new_state = new_state.clone()
            .setCommandContext(dst->getWeakReference())
            .setAccessFlag(0);

        if (src->isPipelineStageSupported(new_state.pipeline_stage))
        {
            sync_object.addSignalOperationToGroup("Release", texture_semaphore_name, new_state.pipeline_stage, 1);
            // External is used as an Acquire synchronization as well
            sync_object.addWaitOperationToGroup(SyncGroups::kExternal, texture_semaphore_name, new_state.pipeline_stage, 1);
            {
                VkCommandBufferBeginInfo src_begin_info{};
                src_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                src_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(src_command_buffer, &src_begin_info);

                // Release And State Transition
                src_state_machine.recordStateChange(texture, new_state);
                src_state_machine.commitChanges(src_command_buffer);

                vkEndCommandBuffer(src_command_buffer);

                VkCommandBufferSubmitInfo src_command_buffer_info{};
                src_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                src_command_buffer_info.commandBuffer = src_command_buffer;

                VkSubmitInfo2 src_submit_info{};
                src_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                src_submit_info.commandBufferInfoCount = 1;

                src_submit_info.pCommandBufferInfos = &src_command_buffer_info;
                auto release_operations = sync_operations.extractOperations(SyncOperations::ExtractWaitOperations)
                    .unionWith(sync_object.getOperationsGroup("Release"));
                release_operations.fillInfo(src_submit_info);
                vkQueueSubmit2(src->getQueue(), 1, &src_submit_info, VK_NULL_HANDLE);
            }
            ResourceStateMachine::resetStages(*texture);
            {
                VkCommandBufferBeginInfo dst_begin_info{};
                dst_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                dst_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(dst_command_buffer, &dst_begin_info);

                // Acquire
                dst_state_machine.recordStateChange(texture, new_state);
                dst_state_machine.commitChanges(dst_command_buffer);

                vkEndCommandBuffer(dst_command_buffer);

                VkCommandBufferSubmitInfo dst_command_buffer_info{};
                dst_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                dst_command_buffer_info.commandBuffer = dst_command_buffer;

                VkSubmitInfo2 dst_submit_info{};
                dst_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                dst_submit_info.commandBufferInfoCount = 1;

                dst_submit_info.pCommandBufferInfos = &dst_command_buffer_info;
                auto external_operations = sync_object.getOperationsGroup(SyncGroups::kExternal).
                    createUnionWith(sync_operations.extractOperations(SyncOperations::ExtractSignalOperations | SyncOperations::ExtractFence));
                external_operations.fillInfo(dst_submit_info);

                vkQueueSubmit2(dst->getQueue(), 1, &dst_submit_info, VK_NULL_HANDLE);
            }
        }
        else
        {
            TextureState original_state = texture->getResourceState().clone()
                .setAccessFlag(0);
            TextureState old_state = texture->getResourceState().clone().
                setCommandContext(dst->getWeakReference())
                .setAccessFlag(0);
            sync_object.addSignalOperationToGroup("Release", texture_semaphore_name, old_state.pipeline_stage, 1);
            // External is used as an Acquire synchronization as well
            sync_object.addWaitOperationToGroup(SyncGroups::kExternal, texture_semaphore_name, old_state.pipeline_stage, 1);
            {
                VkCommandBufferBeginInfo src_begin_info{};
                src_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                src_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(src_command_buffer, &src_begin_info);

                // Release
                src_state_machine.recordStateChange(texture, old_state);
                src_state_machine.commitChanges(src_command_buffer);

                vkEndCommandBuffer(src_command_buffer);

                VkCommandBufferSubmitInfo src_command_buffer_info{};
                src_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                src_command_buffer_info.commandBuffer = src_command_buffer;

                VkSubmitInfo2 src_submit_info{};
                src_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                src_submit_info.commandBufferInfoCount = 1;

                src_submit_info.pCommandBufferInfos = &src_command_buffer_info;
                auto release_operations = sync_operations.extractOperations(SyncOperations::ExtractWaitOperations)
                    .unionWith(sync_object.getOperationsGroup("Release"));
                release_operations.fillInfo(src_submit_info);
                vkQueueSubmit2(src->getQueue(), 1, &src_submit_info, *sync_object.getOperationsGroup(SyncGroups::kExternal).getFence());
                vkWaitForFences(src->getLogicalDevice(), 1, sync_object.getOperationsGroup(SyncGroups::kExternal).getFence(), VK_TRUE, UINT64_MAX);
                vkResetFences(src->getLogicalDevice(), 1, sync_object.getOperationsGroup(SyncGroups::kExternal).getFence());
                texture->overrideResourceState(original_state);
            }

            {
                VkCommandBufferBeginInfo dst_begin_info{};
                dst_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                dst_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(dst_command_buffer, &dst_begin_info);

                // Acquire
                dst_state_machine.recordStateChange(texture, old_state);
                dst_state_machine.commitChanges(dst_command_buffer);
                // State transition
                dst_state_machine.recordStateChange(texture, new_state);
                dst_state_machine.commitChanges(dst_command_buffer);

                vkEndCommandBuffer(dst_command_buffer);

                VkCommandBufferSubmitInfo dst_command_buffer_info{};
                dst_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                dst_command_buffer_info.commandBuffer = dst_command_buffer;

                VkSubmitInfo2 dst_submit_info{};
                dst_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                dst_submit_info.commandBufferInfoCount = 1;

                dst_submit_info.pCommandBufferInfos = &dst_command_buffer_info;
                auto external_operations = sync_object.getOperationsGroup(SyncGroups::kExternal).
                    createUnionWith(sync_operations.extractOperations(SyncOperations::ExtractSignalOperations | SyncOperations::ExtractFence));
                external_operations.fillInfo(dst_submit_info);
                vkQueueSubmit2(dst->getQueue(), 1, &dst_submit_info, *sync_object.getOperationsGroup(SyncGroups::kExternal).getFence());
                vkWaitForFences(src->getLogicalDevice(), 1, sync_object.getOperationsGroup(SyncGroups::kExternal).getFence(), VK_TRUE, UINT64_MAX);

            }
        }
        return sync_object;
    }

    void ResourceStateMachine::commitChanges(VkCommandBuffer command_buffer)
    {
        auto image_barriers = createImageBarriers();
        auto buffer_barriers = createBufferBarriers();
        if (image_barriers.empty() && buffer_barriers.empty())
        {
            return;
        }
        {
            VkDependencyInfo dependency{};
            dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency.imageMemoryBarrierCount = image_barriers.size();
            dependency.pImageMemoryBarriers = image_barriers.data();
            dependency.bufferMemoryBarrierCount = buffer_barriers.size();
            dependency.pBufferMemoryBarriers = buffer_barriers.data();
            vkCmdPipelineBarrier2(command_buffer, &dependency);
        }
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
            if (_apply_barrier_even_without_state_change == false
                && stateCanMakeChangesOnMemory(state_description.access_flag) == false)
            {
                state_description.access_flag = VK_ACCESS_2_NONE_KHR;
                state_description.pipeline_stage = VK_PIPELINE_STAGE_2_NONE;
            }

            if (next_state == std::nullopt
                || (_apply_barrier_even_without_state_change == false && next_state == state_description))
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

            std::optional<uint32_t> current_queue_family_index = state_description.getQueueFamilyIndex();
            std::optional<uint32_t> next_queue_family_index = next_state->getQueueFamilyIndex();
            bool queue_synchronization_required = false;
            assert(current_queue_family_index.has_value() == next_queue_family_index.has_value()
                   && "During family queue ownership transfer both states need a family queue index");

            if (current_queue_family_index != std::nullopt && next_queue_family_index != std::nullopt)
            {
                barrier.srcQueueFamilyIndex = *current_queue_family_index;
                barrier.dstQueueFamilyIndex = *next_queue_family_index;
            }

            barrier.oldLayout = state_description.layout;
            barrier.newLayout = next_state->layout;
            barrier.subresourceRange = texture->createSubresourceRange();
            image_barriers.emplace_back(barrier);
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

            std::optional<uint32_t> current_queue_family_index = state_description.getQueueFamilyIndex();
            std::optional<uint32_t> next_queue_family_index = next_state->getQueueFamilyIndex();

            assert(current_queue_family_index == next_queue_family_index
                   && "Queue family ownership transition should be done by the function transferOwnership");
            barrier.offset = 0;
            barrier.size = buffer->getDeviceSize();
            buffer_barriers.emplace_back(barrier);
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