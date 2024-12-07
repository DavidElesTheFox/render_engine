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
        explicit ResourceStateMachine(LogicalDevice& logical_device,
                                      SubmitScope* scope)
            : _logical_device(logical_device)
            , _current_scope(scope)
        {}
        [[nodiscard]]
        static SyncObject transferOwnership(Texture* texture,
                                            TextureState new_state,
                                            SingleShotCommandBufferFactory* src,
                                            std::shared_ptr<SingleShotCommandBufferFactory> dst,
                                            const SyncOperations& sync_operations,
                                            SubmitScope&& scope,
                                            QueueSubmitTracker* submit_tracker = nullptr);

        [[nodiscard]]
        static SyncObject transferOwnership(Buffer* buffer,
                                            BufferState new_state,
                                            SingleShotCommandBufferFactory* src,
                                            std::shared_ptr<SingleShotCommandBufferFactory> dst,
                                            const SyncOperations& sync_operations,
                                            SubmitScope&& scope,
                                            QueueSubmitTracker* submit_tracker = nullptr);

        [[nodiscard]]
        static SyncObject barrier(Texture& texture,
                                  SingleShotCommandBufferFactory& src,
                                  const SyncOperations& sync_operations,
                                  SubmitScope&& submit_scope,
                                  QueueSubmitTracker* submit_tracker = nullptr);
        [[nodiscard]]
        static SyncObject barrier(Buffer* buffer,
                                  SingleShotCommandBufferFactory* src,
                                  const SyncOperations& sync_operations,
                                  SubmitScope&& submit_scope,
                                  QueueSubmitTracker* submit_tracker = nullptr);
        explicit ResourceStateMachine(LogicalDevice& logical_device)
            : _logical_device(logical_device)
        {}

        void recordStateChange(Texture* texture, TextureState next_state);
        void recordStateChange(Buffer* buffer, BufferState next_state);
        void commitChanges(VkCommandBuffer command_buffer);
    private:
        [[nodiscard]]
        static SyncObject transferOwnershipImpl(ResourceStateHolder auto* texture,
                                                ResourceState auto new_state,
                                                SingleShotCommandBufferFactory* src,
                                                std::shared_ptr<SingleShotCommandBufferFactory> dst,
                                                const SyncOperations& sync_operations,
                                                QueueSubmitTracker* submit_tracker,
                                                SubmitScope&& scope);

        static void ownershipTransformRelease(VkCommandBuffer src_command_buffer,
                                              SingleShotCommandBufferFactory* command_context,
                                              ResourceStateHolder auto* texture,
                                              const ResourceState auto& transition_state,
                                              const SyncObject& transformation_sync_object,
                                              const SyncOperations& external_operations,
                                              QueueSubmitTracker* submit_tracker,
                                              SubmitScope&& scope);

        static void ownershipTransformAcquire(VkCommandBuffer dst_command_buffer,
                                              SingleShotCommandBufferFactory* command_context,
                                              ResourceStateHolder auto* texture,
                                              const ResourceState auto& transition_state,
                                              const SyncObject& transformation_sync_object,
                                              const SyncOperations& external_operations,
                                              const std::function<void(VkCommandBuffer, ResourceStateMachine&)>& additional_command,
                                              QueueSubmitTracker* submit_tracker,
                                              SubmitScope&& scope);

        static SyncObject barrierImpl(ResourceStateHolder auto& resource,
                                      SingleShotCommandBufferFactory& src,
                                      const SyncOperations& sync_operations,
                                      QueueSubmitTracker* submit_tracker,
                                      SubmitScope&& scope);


        std::vector<VkImageMemoryBarrier2> createImageBarriers();
        std::vector<VkBufferMemoryBarrier2> createBufferBarriers();
        bool stateCanMakeChangesOnMemory(VkAccessFlags2 access);

        LogicalDevice& _logical_device;
        std::unordered_map<Texture*, TextureState> _images{};
        std::unordered_map<Buffer*, BufferState> _buffers{};
        SubmitScope* _current_scope{ nullptr };
    };

}
