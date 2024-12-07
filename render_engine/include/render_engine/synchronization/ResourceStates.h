#pragma once
#include <volk.h>

#include <render_engine/CommandContext.h>


#include <memory>
#include <type_traits>

namespace RenderEngine
{
    class ResourceAccessToken
    {
        friend class ResourceStateMachine;
        friend class RenderPass;
        friend class Texture;
        friend class Buffer;
    private:
        ResourceAccessToken() = default;
    };
    // TODO put it its own file
    class SubmitScope
    {
        static inline std::atomic_uint32_t kNextId{ 0 };
        static inline auto kCompareCommandBuffer = [](VkCommandBuffer a, VkCommandBuffer b) { return reinterpret_cast<void*>(a) < reinterpret_cast<void*>(b); };
    public:
        SubmitScope()
            : _id(kNextId++)
        {}
        ~SubmitScope()
        {
            for (auto& callback : _cleanup_callbacks)
            {
                callback(this);
            }
        }
        SubmitScope(SubmitScope&&) = default;
        SubmitScope(const SubmitScope&) = delete;

        SubmitScope& operator=(SubmitScope&&) = default;
        SubmitScope& operator=(const SubmitScope&) = delete;

        void assignCommandBuffer(VkCommandBuffer command_buffer)
        {
            _assigned_command_buffers.push_back(command_buffer);
            std::ranges::sort(_assigned_command_buffers,
                              kCompareCommandBuffer);
        }

        bool hasCommandBuffer(VkCommandBuffer command_buffer) const
        {
            return std::ranges::binary_search(_assigned_command_buffers,
                                              command_buffer,
                                              kCompareCommandBuffer);
        }

        uint64_t getId() const { return _id; }
        void addCleanUpFunction(std::function<void(const SubmitScope* submit_scope)> callback)
        {
            _cleanup_callbacks.push_back(std::move(callback));
        }
    private:
        const uint32_t _id;
        std::vector<VkCommandBuffer> _assigned_command_buffers;
        std::vector<std::function<void(const SubmitScope* submit_scope)>> _cleanup_callbacks;
    };

    struct TextureState
    {
        enum DirtyFlags : uint32_t
        {
            None = 0,
            PipelineStage = 1 << 1,
            AccessFlag = 1 << 2,
            Layout = 1 << 3,
            CommandContext = 1 << 4
        };


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
        std::weak_ptr<SingleShotCommandBufferFactory> command_context{ };
        uint32_t dirty_flags{ DirtyFlags::None };

        TextureState&& setPipelineStage(VkPipelineStageFlagBits2 value)&&
        {
            this->pipeline_stage = value;
            dirty_flags |= DirtyFlags::PipelineStage;
            return std::move(*this);
        }
        TextureState&& setAccessFlag(VkAccessFlags2 value)&&
        {
            this->access_flag = value;
            dirty_flags |= DirtyFlags::AccessFlag;
            return std::move(*this);
        }
        TextureState&& setImageLayout(VkImageLayout value)&&
        {
            this->layout = value;
            dirty_flags |= DirtyFlags::Layout;
            return std::move(*this);
        }
        TextureState&& setCommandContext(std::weak_ptr<SingleShotCommandBufferFactory> value)&&
        {
            this->command_context = value;
            dirty_flags |= DirtyFlags::CommandContext;
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
            return command_context.expired() ? std::nullopt : std::optional{ command_context.lock()->getQueue().getQueueFamilyIndex() };
        }
    };

    struct BufferState
    {
        enum DirtyFlags : uint32_t
        {
            None = 0,
            PipelineStage = 1 << 1,
            AccessFlag = 1 << 2,
            CommandContext = 1 << 3
        };

        VkPipelineStageFlagBits2 pipeline_stage{ VK_PIPELINE_STAGE_2_NONE };
        VkAccessFlags2 access_flag{ VK_ACCESS_2_NONE };
        // TODO: Implement borrow_ptr.
        std::weak_ptr<SingleShotCommandBufferFactory> command_context{ };
        uint32_t dirty_flags{ DirtyFlags::None };

        BufferState&& setPipelineStage(VkPipelineStageFlagBits2 value)&&
        {
            this->pipeline_stage = value;
            dirty_flags |= DirtyFlags::PipelineStage;
            return std::move(*this);
        }

        BufferState&& setAccessFlag(VkAccessFlags2 value)&&
        {
            this->access_flag = value;
            dirty_flags |= DirtyFlags::AccessFlag;
            return std::move(*this);
        }


        BufferState&& setCommandContext(std::weak_ptr<SingleShotCommandBufferFactory> value)&&
        {
            this->command_context = value;
            dirty_flags |= DirtyFlags::CommandContext;
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
            return command_context.expired() ? std::nullopt : std::optional{ command_context.lock()->getQueue().getQueueFamilyIndex() };
        }
    };
    template<typename T>
    concept ResourceState =
        std::is_same_v<decltype(std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<T>>>::pipeline_stage), VkPipelineStageFlagBits2>
        && std::is_same_v<decltype(std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<T>>>::access_flag), VkAccessFlags2>
        && std::is_same_v<decltype(std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<T>>>::command_context), std::weak_ptr<SingleShotCommandBufferFactory>>;

    template<typename T>
    concept TextureStateWriter = requires(T t, TextureState texture_state, SubmitScope * scope, ResourceAccessToken access_token)
    {
        { t.overrideResourceState(texture_state, scope, access_token) };
    };

    template<typename T>
    concept BufferStateWriter = requires(T t, BufferState buffer_state, SubmitScope * scope, ResourceAccessToken access_token)
    {
        { t.overrideResourceState(buffer_state, scope, access_token) };
    };
    template<typename T>
    concept ResourceStateHolder = requires(T t, SubmitScope * scope)
    {
        { t.getResourceState(scope) } -> ResourceState;
        TextureStateWriter<T> || BufferStateWriter<T>;
    };

    static_assert(ResourceState<TextureState>, "Texture state must fulfill the resource state requirements");
    static_assert(ResourceState<BufferState>, "Buffer state must fulfill the resource state requirements");
}
