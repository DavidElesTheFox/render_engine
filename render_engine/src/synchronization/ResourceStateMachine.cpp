#include <render_engine/synchronization/ResourceStateMachine.h>

#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

#include <cassert>
#include <stdexcept>
namespace RenderEngine
{
    namespace SyncGroups
    {
        constexpr auto* kRelease = "Release";
        constexpr auto* kAcquire = "Acquire";
    }
    void ResourceStateMachine::resetStages(Texture& texture)
    {
        texture.overrideResourceState(texture.getResourceState().clone()
                                      .setPipelineStage(VK_PIPELINE_STAGE_2_NONE)
                                      .setAccessFlag(VK_ACCESS_2_NONE), {});
    }

    void ResourceStateMachine::resetStages(Buffer& texture)
    {
        texture.overrideResourceState(texture.getResourceState().clone()
                                      .setPipelineStage(VK_PIPELINE_STAGE_2_NONE)
                                      .setAccessFlag(VK_ACCESS_2_NONE), {});
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

    SyncObject ResourceStateMachine::transferOwnershipImpl(ResourceStateHolder auto* resource,
                                                           ResourceState auto new_state,
                                                           SingleShotCommandContext* src,
                                                           SingleShotCommandContext* dst,
                                                           const SyncOperations& sync_operations)
    {
        auto src_command_buffer = src->createCommandBuffer();
        auto dst_command_buffer = dst->createCommandBuffer();

        assert(*src->getLogicalDevice() == *dst->getLogicalDevice());
        assert(dst->isPipelineStageSupported(new_state.pipeline_stage));
        SyncObject sync_object = SyncObject::CreateEmpty(src->getLogicalDevice());

        const std::string texture_semaphore_name = std::format("{:#x}", reinterpret_cast<intptr_t>(resource));
        sync_object.createTimelineSemaphore(texture_semaphore_name, 0, 2);

        // make ownership transform
        new_state = new_state.clone()
            .setCommandContext(dst->getWeakReference())
            .setAccessFlag(0); // Access flag is ignored during queue family transition

        if (src->isPipelineStageSupported(new_state.pipeline_stage))
        {
            sync_object.addSignalOperationToGroup(SyncGroups::kRelease, texture_semaphore_name, new_state.pipeline_stage, 1);
            sync_object.addWaitOperationToGroup(SyncGroups::kAcquire, texture_semaphore_name, new_state.pipeline_stage, 1);
            ownershipTransformRelease(src_command_buffer,
                                      src->getQueue(),
                                      src->getLogicalDevice(),
                                      resource,
                                      new_state,
                                      sync_object,
                                      sync_operations.restrict(*src));
            ownershipTransformAcquire(dst_command_buffer,
                                      dst->getQueue(),
                                      dst->getLogicalDevice(),
                                      resource,
                                      new_state,
                                      sync_object,
                                      sync_operations.restrict(*dst),
                                      nullptr);
        }
        else
        {
            /*
            * Because the source stage doesn't support the required state in the destination:
            *  - First we are making the ownership transition with the original state
            *  - Then after the Acquire operation we do the other state transition.
            */
            auto transition_state = resource->getResourceState().clone().
                setCommandContext(dst->getWeakReference())
                .setAccessFlag(0); // Access flag is ignored during queue family transition

            sync_object.addSignalOperationToGroup(SyncGroups::kRelease, texture_semaphore_name, transition_state.pipeline_stage, 1);
            sync_object.addWaitOperationToGroup(SyncGroups::kAcquire, texture_semaphore_name, transition_state.pipeline_stage, 1);

            auto extra_state_transition = [&](VkCommandBuffer command_buffer, ResourceStateMachine& state_machine)
                {
                    state_machine.recordStateChange(resource, new_state);
                    state_machine.commitChanges(command_buffer);
                };

            ownershipTransformRelease(src_command_buffer,
                                      src->getQueue(),
                                      src->getLogicalDevice(),
                                      resource,
                                      transition_state,
                                      sync_object,
                                      sync_operations.restrict(*src));
            ownershipTransformAcquire(dst_command_buffer,
                                      dst->getQueue(),
                                      dst->getLogicalDevice(),
                                      resource,
                                      transition_state,
                                      sync_object,
                                      sync_operations.restrict(*dst),
                                      extra_state_transition);
        }
        return sync_object;
    }

    void ResourceStateMachine::ownershipTransformRelease(VkCommandBuffer src_command_buffer,
                                                         VkQueue src_queue,
                                                         LogicalDevice& logical_device,
                                                         ResourceStateHolder auto* resource,
                                                         const ResourceState auto& transition_state,
                                                         const SyncObject& transformation_sync_object,
                                                         const SyncOperations& external_operations)
    {
        ResourceStateMachine src_state_machine(logical_device);

        VkCommandBufferBeginInfo src_begin_info{};
        src_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        src_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        logical_device->vkBeginCommandBuffer(src_command_buffer, &src_begin_info);

        // Release - Now we are making a queue family ownership transformation. For this we don't need to change the state
        src_state_machine.recordStateChange(resource, transition_state);
        src_state_machine.commitChanges(src_command_buffer, false);

        logical_device->vkEndCommandBuffer(src_command_buffer);

        VkCommandBufferSubmitInfo src_command_buffer_info{};
        src_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        src_command_buffer_info.commandBuffer = src_command_buffer;

        VkSubmitInfo2 src_submit_info{};
        src_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        src_submit_info.commandBufferInfoCount = 1;

        src_submit_info.pCommandBufferInfos = &src_command_buffer_info;
        auto release_operations =
            transformation_sync_object.query()
            .select(SyncGroups::kRelease)
            .join(external_operations.extract(SyncOperations::ExtractWaitOperations)).get();
        release_operations.fillInfo(src_submit_info);
        logical_device->vkQueueSubmit2(src_queue, 1, &src_submit_info, VK_NULL_HANDLE);
    }

    void ResourceStateMachine::ownershipTransformAcquire(VkCommandBuffer dst_command_buffer,
                                                         VkQueue dst_queue,
                                                         LogicalDevice& logical_device,
                                                         ResourceStateHolder auto* resource,
                                                         const ResourceState auto& transition_state,
                                                         const SyncObject& transformation_sync_object,
                                                         const SyncOperations& external_operations,
                                                         const std::function<void(VkCommandBuffer, ResourceStateMachine&)>& additional_command)
    {
        ResourceStateMachine dst_state_machine(logical_device);

        VkCommandBufferBeginInfo dst_begin_info{};
        dst_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        dst_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        logical_device->vkBeginCommandBuffer(dst_command_buffer, &dst_begin_info);

        // Acquire - Here the state machine can make the state change
        dst_state_machine.recordStateChange(resource, transition_state);
        dst_state_machine.commitChanges(dst_command_buffer);

        if (additional_command != nullptr)
        {
            additional_command(dst_command_buffer, dst_state_machine);
        }
        logical_device->vkEndCommandBuffer(dst_command_buffer);

        VkCommandBufferSubmitInfo dst_command_buffer_info{};
        dst_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        dst_command_buffer_info.commandBuffer = dst_command_buffer;

        VkSubmitInfo2 dst_submit_info{};
        dst_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        dst_submit_info.commandBufferInfoCount = 1;

        dst_submit_info.pCommandBufferInfos = &dst_command_buffer_info;
        auto acquire_operations = transformation_sync_object.query()
            .select({ SyncGroups::kInternal, SyncGroups::kAcquire })
            .join(external_operations.extract(SyncOperations::ExtractSignalOperations | SyncOperations::ExtractFence)).get();
        acquire_operations.fillInfo(dst_submit_info);
        logical_device->vkQueueSubmit2(dst_queue, 1, &dst_submit_info, acquire_operations.getFence());
    }

    [[nodiscard]]
    SyncObject ResourceStateMachine::transferOwnership(Texture* texture,
                                                       TextureState new_state,
                                                       SingleShotCommandContext* src,
                                                       SingleShotCommandContext* dst,
                                                       const SyncOperations& sync_operations)
    {
        return transferOwnershipImpl(texture,
                                     new_state,
                                     src,
                                     dst,
                                     sync_operations);
    }
    [[nodiscard]]
    SyncObject ResourceStateMachine::transferOwnership(Buffer* buffer,
                                                       BufferState new_state,
                                                       SingleShotCommandContext* src,
                                                       SingleShotCommandContext* dst,
                                                       const SyncOperations& sync_operations)
    {
        return transferOwnershipImpl(buffer,
                                     new_state,
                                     src,
                                     dst,
                                     sync_operations);
    }

    SyncObject ResourceStateMachine::barrier(Texture& texture, SingleShotCommandContext& src, const SyncOperations& sync_operations)
    {
        return barrierImpl(texture, src, sync_operations);
    }
    SyncObject ResourceStateMachine::barrier(Buffer* buffer, SingleShotCommandContext* src, const SyncOperations& sync_operations)
    {
        return barrierImpl(*buffer, *src, sync_operations);
    }
    SyncObject ResourceStateMachine::barrierImpl(ResourceStateHolder auto& resource,
                                                 SingleShotCommandContext& src,
                                                 const SyncOperations& sync_operations)
    {
        SyncObject result = SyncObject::CreateEmpty(src.getLogicalDevice());
        result.createTimelineSemaphore("BarrierFinished", 0, 2);
        result.addSignalOperationToGroup(SyncGroups::kInternal,
                                         "BarrierFinished",
                                         resource.getResourceState().pipeline_stage,
                                         1);
        result.addWaitOperationToGroup(SyncGroups::kExternal,
                                       "BarrierFinished",
                                       resource.getResourceState().pipeline_stage,
                                       1);

        auto command_buffer = src.createCommandBuffer();
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        src.getLogicalDevice()->vkBeginCommandBuffer(command_buffer, &begin_info);

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = 0;
        dependency.bufferMemoryBarrierCount = 0;
        src.getLogicalDevice()->vkCmdPipelineBarrier2(command_buffer, &dependency);

        src.getLogicalDevice()->vkEndCommandBuffer(command_buffer);

        VkCommandBufferSubmitInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.commandBuffer = command_buffer;

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.commandBufferInfoCount = 1;

        submit_info.pCommandBufferInfos = &command_buffer_info;
        auto operations =
            result.query()
            .select(SyncGroups::kInternal)
            .join(sync_operations.extract(SyncOperations::ExtractWaitOperations)).get();
        operations.fillInfo(submit_info);
        src.getLogicalDevice()->vkQueueSubmit2(src.getQueue(), 1, &submit_info, VK_NULL_HANDLE);
        return result;
    }

    void ResourceStateMachine::commitChanges(VkCommandBuffer command_buffer, bool apply_state_change_on_objects)
    {
        auto image_barriers = createImageBarriers(apply_state_change_on_objects);
        auto buffer_barriers = createBufferBarriers(apply_state_change_on_objects);
        if (image_barriers.empty() && buffer_barriers.empty())
        {
            return;
        }
        {
            VkDependencyInfo dependency{};
            dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size());
            dependency.pImageMemoryBarriers = image_barriers.data();
            dependency.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size());
            dependency.pBufferMemoryBarriers = buffer_barriers.data();
            _logical_device->vkCmdPipelineBarrier2(command_buffer, &dependency);
        }
    }


    std::vector<VkImageMemoryBarrier2> ResourceStateMachine::createImageBarriers(bool apply_state_change_on_texture)
    {
        std::vector<VkImageMemoryBarrier2> image_barriers;
        for (auto& [texture, next_state] : _images)
        {
            auto state_description = texture->getResourceState();

            if (next_state == state_description)
            {
                continue;
            }
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = texture->getVkImage();
            barrier.srcStageMask = state_description.pipeline_stage;
            barrier.srcAccessMask = state_description.access_flag;
            barrier.dstStageMask = next_state.pipeline_stage;
            barrier.dstAccessMask = next_state.access_flag;

            std::optional<uint32_t> current_queue_family_index = state_description.getQueueFamilyIndex();
            std::optional<uint32_t> next_queue_family_index = next_state.getQueueFamilyIndex();
            assert(current_queue_family_index.has_value() == next_queue_family_index.has_value()
                   && "During family queue ownership transfer both states need a family queue index");

            if (current_queue_family_index != std::nullopt && next_queue_family_index != std::nullopt)
            {
                barrier.srcQueueFamilyIndex = *current_queue_family_index;
                barrier.dstQueueFamilyIndex = *next_queue_family_index;
            }

            barrier.oldLayout = state_description.layout;
            barrier.newLayout = next_state.layout;
            barrier.subresourceRange = texture->createSubresourceRange();
            image_barriers.emplace_back(barrier);
            if (apply_state_change_on_texture)
            {
                texture->overrideResourceState(next_state, {});
            }
        }
        _images.clear();
        return image_barriers;
    }

    std::vector<VkBufferMemoryBarrier2> ResourceStateMachine::createBufferBarriers(bool apply_state_change_on_buffer)
    {
        std::vector<VkBufferMemoryBarrier2> buffer_barriers;
        for (auto& [buffer, next_state] : _buffers)
        {
            auto state_description = buffer->getResourceState();

            if (next_state == state_description)
            {
                continue;
            }
            VkBufferMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.buffer = buffer->getBuffer();

            barrier.srcStageMask = state_description.pipeline_stage;
            barrier.srcAccessMask = state_description.access_flag;

            barrier.dstStageMask = next_state.pipeline_stage;
            barrier.dstAccessMask = next_state.access_flag;

            std::optional<uint32_t> current_queue_family_index = state_description.getQueueFamilyIndex();
            std::optional<uint32_t> next_queue_family_index = next_state.getQueueFamilyIndex();

            assert(current_queue_family_index.has_value() == next_queue_family_index.has_value()
                   && "During family queue ownership transfer both states need a family queue index");
            if (current_queue_family_index != std::nullopt && next_queue_family_index != std::nullopt)
            {
                barrier.srcQueueFamilyIndex = *current_queue_family_index;
                barrier.dstQueueFamilyIndex = *next_queue_family_index;
            }
            barrier.offset = 0;
            barrier.size = buffer->getDeviceSize();
            buffer_barriers.emplace_back(barrier);
            if (apply_state_change_on_buffer)
            {
                buffer->overrideResourceState(next_state, {});
            }
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