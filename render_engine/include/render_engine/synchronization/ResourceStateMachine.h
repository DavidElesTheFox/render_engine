#pragma once

#include <volk.h>

#include <render_engine/CommandContext.h>
#include <render_engine/synchronization/SyncObject.h>

#include <cassert>
#include <memory>
#include <optional>
#include <unordered_map>

namespace RenderEngine
{
    class Buffer;
    class Texture;
    class ResourceStateMachine
    {
    public:
        struct TextureState
        {
            VkPipelineStageFlagBits2 pipeline_stage{ VK_PIPELINE_STAGE_2_NONE };
            VkAccessFlags2 access_flag{ VK_ACCESS_2_NONE };
            VkImageLayout layout{ VK_IMAGE_LAYOUT_UNDEFINED };
            /* TODO: Implement borrow_ptr.
            * Textures lifetime is managed by the application. Thus, storing a raw pointer to a command context is not
            * a good solution. No one can guarantee that a texture doesn't have more lifetime then a transfer engine for example.
            *
            * For this solution a shared_ptr/weak_ptr is a solution but the ownership question is not clear. It is because the
            * object is not shared, it has only one owner and many references.
            *
            * The solution for this a borrowed_ptr concept what Rust also has.
            */
            std::weak_ptr<CommandContext> command_context{ };

            TextureState&& setPipelineStage(VkPipelineStageFlagBits2 pipeline_stage)&&
            {
                this->pipeline_stage = pipeline_stage;
                return std::move(*this);
            }
            TextureState&& setAccessFlag(VkAccessFlags2 access_flag)&&
            {
                this->access_flag = access_flag;
                return std::move(*this);
            }
            TextureState&& setImageLayout(VkImageLayout layout)&&
            {
                this->layout = layout;
                return std::move(*this);
            }
            TextureState&& setCommandContext(std::weak_ptr<CommandContext> command_context)&&
            {
                this->command_context = command_context;
                return std::move(*this);
            }

            TextureState clone() const
            {
                return *this;
            }

            bool operator==(const TextureState& o) const
            {
                return pipeline_stage == o.pipeline_stage
                    && access_flag == o.access_flag
                    && layout == o.layout
                    && hasSameCommandContext(o);
            }

            bool hasSameCommandContext(const TextureState& o) const
            {
                return (command_context.expired() && o.command_context.expired())
                    || (command_context.expired() == false
                        && o.command_context.expired() == false
                        && command_context.lock().get() == o.command_context.lock().get());
            }
            bool operator!=(const TextureState& o) const
            {
                return ((*this) == o) == false;
            }
            std::optional<uint32_t> getQueueFamilyIndex() const
            {
                return command_context.expired() ? std::nullopt : std::optional{ command_context.lock()->getQueueFamilyIndex() };
            }
        };

        struct BufferState
        {
            VkPipelineStageFlagBits2 pipeline_stage{ VK_PIPELINE_STAGE_2_NONE };
            VkAccessFlags2 access_flag{ VK_ACCESS_2_NONE };
            // TODO: Implement borrow_ptr.
            std::weak_ptr<CommandContext> command_context{ };

            BufferState&& setPipelineStage(VkPipelineStageFlagBits2 pipeline_stage)&&
            {
                this->pipeline_stage = pipeline_stage;
                return std::move(*this);
            }

            BufferState&& setAccessFlag(VkAccessFlags2 access_flag)&&
            {
                this->access_flag = access_flag;
                return std::move(*this);
            }


            BufferState&& setCommandContext(std::weak_ptr<CommandContext> command_context)&&
            {
                this->command_context = command_context;
                return std::move(*this);
            }

            BufferState clone() const
            {
                return *this;
            }
            bool operator==(const BufferState& o) const
            {
                return pipeline_stage == o.pipeline_stage
                    && access_flag == o.access_flag
                    && hasSameCommandContext(o);
            }

            bool hasSameCommandContext(const BufferState& o) const
            {
                return (command_context.expired() && o.command_context.expired())
                    || (command_context.expired() == false
                        && o.command_context.expired() == false
                        && command_context.lock().get() == o.command_context.lock().get());
            }

            bool operator!=(const BufferState& o) const
            {
                return ((*this) == o) == false;
            }
            std::optional<uint32_t> getQueueFamilyIndex() const
            {
                return command_context.expired() ? std::nullopt : std::optional{ command_context.lock()->getQueueFamilyIndex() };
            }
        };

        static void resetStages(Texture& texture);
        static void resetStages(Buffer& texture);
        [[nodiscard]]
        static SyncObject transferOwnership(Texture* texture,
                                            TextureState new_state,
                                            CommandContext* src,
                                            CommandContext* dst,
                                            const SyncOperations& sync_operations);

        ResourceStateMachine() = default;

        void recordStateChange(Texture* texture, TextureState next_state);
        void recordStateChange(Buffer* buffer, BufferState next_state);
        void commitChanges(VkCommandBuffer command_buffer);
    private:
        std::vector<VkImageMemoryBarrier2> createImageBarriers();
        std::vector<VkBufferMemoryBarrier2> createBufferBarriers();
        bool stateCanMakeChangesOnMemory(VkAccessFlags2 access);

        // TODO remove optional - so far it always has value
        std::unordered_map<Texture*, std::optional<TextureState>> _images{};
        std::unordered_map<Buffer*, std::optional<BufferState>> _buffers{};
        // TODO remove it
        bool _apply_barrier_even_without_state_change{ false };

    };
}