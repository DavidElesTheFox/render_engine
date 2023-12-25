#pragma once

#include <span>
#include <volk.h>

namespace RenderEngine
{
    class PushConstantsUpdater
    {
    public:
        PushConstantsUpdater(VkCommandBuffer command_buffer,
                             VkPipelineLayout layout)
            : _command_buffer(command_buffer)
            , _layout(layout)
        {}
        PushConstantsUpdater(const PushConstantsUpdater&) = delete;
        PushConstantsUpdater(PushConstantsUpdater&&) = default;

        PushConstantsUpdater& operator=(const PushConstantsUpdater&) = delete;
        PushConstantsUpdater& operator=(PushConstantsUpdater&&) = default;

        void update(VkShaderStageFlags shader_stages, size_t offset, std::span<const uint8_t> data);
    private:
        VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
        VkPipelineLayout _layout{ VK_NULL_HANDLE };
    };
}