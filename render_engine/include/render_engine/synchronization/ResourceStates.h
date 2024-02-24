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
    private:
        ResourceAccessToken() = default;
    };
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

        TextureState&& setPipelineStage(VkPipelineStageFlagBits2 value)&&
        {
            this->pipeline_stage = value;
            return std::move(*this);
        }
        TextureState&& setAccessFlag(VkAccessFlags2 value)&&
        {
            this->access_flag = value;
            return std::move(*this);
        }
        TextureState&& setImageLayout(VkImageLayout value)&&
        {
            this->layout = value;
            return std::move(*this);
        }
        TextureState&& setCommandContext(std::weak_ptr<CommandContext> value)&&
        {
            this->command_context = value;
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

        BufferState&& setPipelineStage(VkPipelineStageFlagBits2 value)&&
        {
            this->pipeline_stage = value;
            return std::move(*this);
        }

        BufferState&& setAccessFlag(VkAccessFlags2 value)&&
        {
            this->access_flag = value;
            return std::move(*this);
        }


        BufferState&& setCommandContext(std::weak_ptr<CommandContext> value)&&
        {
            this->command_context = value;
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
    template<typename T>
    concept ResourceState =
        std::is_same_v<decltype(std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<T>>>::pipeline_stage), VkPipelineStageFlagBits2>
        && std::is_same_v<decltype(std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<T>>>::access_flag), VkAccessFlags2>
        && std::is_same_v<decltype(std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<T>>>::command_context), std::weak_ptr<CommandContext>>;

    template<typename T>
    concept TextureStateWriter = requires(T t, TextureState texture_state, ResourceAccessToken access_token)
    {
        { t.overrideResourceState(texture_state, access_token) };
    };

    template<typename T>
    concept BufferStateWriter = requires(T t, BufferState buffer_state, ResourceAccessToken access_token)
    {
        { t.overrideResourceState(buffer_state, access_token) };
    };
    template<typename T>
    concept ResourceStateHolder = requires(const T const_t)
    {
        { const_t.getResourceState() } -> ResourceState;
        TextureStateWriter<T> || BufferStateWriter<T>;
    };

    template<typename T>
    struct IsResourceState
    {
        static const bool value = false;
    };

    template<ResourceState T>
    struct IsResourceState<T>
    {
        static const bool value = true;
    };

    template<typename T>
    constexpr bool IsResourceState_V = IsResourceState<T>::value;

    template<typename T>
    struct IsResourceStateHolder
    {
        static const bool value = false;
    };

    template<ResourceStateHolder T>
    struct IsResourceStateHolder<T>
    {
        static const bool value = true;
    };

    template<typename T>
    constexpr bool IsResourceStateHolder_V = IsResourceStateHolder<T>::value;

    static_assert(IsResourceState_V<TextureState>, "Texture state must fulfill the resource state requirements");
    static_assert(IsResourceState_V<BufferState>, "Buffer state must fulfill the resource state requirements");
}