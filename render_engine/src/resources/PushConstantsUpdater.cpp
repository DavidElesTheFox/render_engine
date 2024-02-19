#include <render_engine/resources/PushConstantsUpdater.h>

namespace RenderEngine
{
    void PushConstantsUpdater::update(VkShaderStageFlags shader_stages, size_t offset, std::span<const uint8_t> data)
    {
        _logical_device->vkCmdPushConstants(_command_buffer,
                                            _layout,
                                            shader_stages,
                                            offset,
                                            data.size(),
                                            data.data());
    }
}