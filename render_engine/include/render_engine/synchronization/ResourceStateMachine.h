#pragma once


#include <render_engine/CommandContext.h>
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
                                            CommandContext* src,
                                            CommandContext* dst,
                                            const SyncOperations& sync_operations);

        [[nodiscard]]
        static SyncObject transferOwnership(Buffer* buffer,
                                            BufferState new_state,
                                            CommandContext* src,
                                            CommandContext* dst,
                                            const SyncOperations& sync_operations);

        [[nodiscard]]
        static SyncObject barrier(Texture* texture,
                                  CommandContext* src,
                                  const SyncOperations& sync_operations);
        [[nodiscard]]
        static SyncObject barrier(Buffer* buffer,
                                  CommandContext* src,
                                  const SyncOperations& sync_operations);
        ResourceStateMachine() = default;

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
                                                CommandContext* src,
                                                CommandContext* dst,
                                                const SyncOperations& sync_operations);

        static void ownershipTransformRelease(VkCommandBuffer src_command_buffer,
                                              VkQueue src_queue,
                                              ResourceStateHolder auto* texture,
                                              const ResourceState auto& transition_state,
                                              const SyncObject& transformation_sync_object,
                                              const SyncOperations& external_operations);

        static void ownershipTransformAcquire(VkCommandBuffer dst_command_buffer,
                                              VkQueue dst_queue,
                                              ResourceStateHolder auto* texture,
                                              const ResourceState auto& transition_state,
                                              const SyncObject& transformation_sync_object,
                                              const SyncOperations& external_operations,
                                              const std::function<void(VkCommandBuffer, ResourceStateMachine&)>& additional_command);

        static SyncObject barrierImpl(ResourceStateHolder auto* resource,
                                      CommandContext* src,
                                      const SyncOperations& sync_operations);

        void commitChanges(VkCommandBuffer command_buffer, bool apply_state_change_on_objects);

        std::vector<VkImageMemoryBarrier2> createImageBarriers(bool apply_state_change_on_texture);
        std::vector<VkBufferMemoryBarrier2> createBufferBarriers(bool apply_state_change_on_buffer);
        bool stateCanMakeChangesOnMemory(VkAccessFlags2 access);

        // TODO remove optional - so far it always has value
        std::unordered_map<Texture*, std::optional<TextureState>> _images{};
        std::unordered_map<Buffer*, std::optional<BufferState>> _buffers{};

    };
}