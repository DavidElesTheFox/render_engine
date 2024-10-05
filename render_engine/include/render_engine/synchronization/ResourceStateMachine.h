#pragma once


#include <render_engine/CommandContext.h>
#include <render_engine/QueueSubmitTracker.h>
#include <render_engine/synchronization/ResourceStates.h>
#include <render_engine/synchronization/SyncObject.h>

#include <cassert>
#include <functional>
#include <optional>
#include <unordered_map>

namespace RenderEngine
{
    class Buffer;
    class Texture;


    class ResourceStateMachine
    {
    public:
        static void resetStages(Texture& texture);
        static void resetStages(Buffer& texture);
        [[nodiscard]]
        static SyncObject transferOwnership(Texture* texture,
                                            TextureState new_state,
                                            SingleShotCommandBufferFactory* src,
                                            std::shared_ptr<SingleShotCommandBufferFactory> dst,
                                            const SyncOperations& sync_operations,
                                            QueueSubmitTracker* submit_tracker = nullptr);

        [[nodiscard]]
        static SyncObject transferOwnership(Buffer* buffer,
                                            BufferState new_state,
                                            SingleShotCommandBufferFactory* src,
                                            std::shared_ptr<SingleShotCommandBufferFactory> dst,
                                            const SyncOperations& sync_operations,
                                            QueueSubmitTracker* submit_tracker = nullptr);

        [[nodiscard]]
        static SyncObject barrier(Texture& texture,
                                  SingleShotCommandBufferFactory& src,
                                  const SyncOperations& sync_operations,
                                  QueueSubmitTracker* submit_tracker = nullptr);
        [[nodiscard]]
        static SyncObject barrier(Buffer* buffer,
                                  SingleShotCommandBufferFactory* src,
                                  const SyncOperations& sync_operations,
                                  QueueSubmitTracker* submit_tracker = nullptr);
        explicit ResourceStateMachine(LogicalDevice& logical_device)
            : _logical_device(logical_device)
        {}

        void recordStateChange(Texture* texture, TextureState next_state);
        void recordStateChange(Buffer* buffer, BufferState next_state);
        void commitChanges(VkCommandBuffer command_buffer)
        {
            commitChanges(command_buffer, true);
        }
    private:
        [[nodiscard]]
        static SyncObject transferOwnershipImpl(ResourceStateHolder auto* texture,
                                                ResourceState auto new_state,
                                                SingleShotCommandBufferFactory* src,
                                                std::shared_ptr<SingleShotCommandBufferFactory> dst,
                                                const SyncOperations& sync_operations,
                                                QueueSubmitTracker* submit_tracker);

        static void ownershipTransformRelease(VkCommandBuffer src_command_buffer,
                                              SingleShotCommandBufferFactory* command_context,
                                              ResourceStateHolder auto* texture,
                                              const ResourceState auto& transition_state,
                                              const SyncObject& transformation_sync_object,
                                              const SyncOperations& external_operations,
                                              QueueSubmitTracker* submit_tracker);

        static void ownershipTransformAcquire(VkCommandBuffer dst_command_buffer,
                                              SingleShotCommandBufferFactory* command_context,
                                              ResourceStateHolder auto* texture,
                                              const ResourceState auto& transition_state,
                                              const SyncObject& transformation_sync_object,
                                              const SyncOperations& external_operations,
                                              const std::function<void(VkCommandBuffer, ResourceStateMachine&)>& additional_command,
                                              QueueSubmitTracker* submit_tracker);

        static SyncObject barrierImpl(ResourceStateHolder auto& resource,
                                      SingleShotCommandBufferFactory& src,
                                      const SyncOperations& sync_operations,
                                      QueueSubmitTracker* submit_tracker);

        void commitChanges(VkCommandBuffer command_buffer, bool apply_state_change_on_objects);

        std::vector<VkImageMemoryBarrier2> createImageBarriers(bool apply_state_change_on_texture);
        std::vector<VkBufferMemoryBarrier2> createBufferBarriers(bool apply_state_change_on_buffer);
        bool stateCanMakeChangesOnMemory(VkAccessFlags2 access);

        LogicalDevice& _logical_device;
        std::unordered_map<Texture*, TextureState> _images{};
        std::unordered_map<Buffer*, BufferState> _buffers{};

    };
}