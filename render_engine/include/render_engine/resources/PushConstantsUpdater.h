#pragma once

#include <volk.h>

#include <render_engine/LogicalDevice.h>

#include <span>

namespace RenderEngine
{
    class PushConstantsUpdater
    {
    public:
        PushConstantsUpdater(LogicalDevice& logical_device,
                             VkCommandBuffer command_buffer,
                             VkPipelineLayout layout)
            : _logical_device(logical_device)
            , _command_buffer(command_buffer)
            , _layout(layout)
        {}
        PushConstantsUpdater(const PushConstantsUpdater&) = delete;
        PushConstantsUpdater(PushConstantsUpdater&&) = default;

        PushConstantsUpdater& operator=(const PushConstantsUpdater&) = delete;
        PushConstantsUpdater& operator=(PushConstantsUpdater&&) = default;

        void update(VkShaderStageFlags shader_stages, size_t offset, std::span<const uint8_t> data);
    private:
        LogicalDevice& _logical_device;
        VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
        VkPipelineLayout _layout{ VK_NULL_HANDLE };
    };
}