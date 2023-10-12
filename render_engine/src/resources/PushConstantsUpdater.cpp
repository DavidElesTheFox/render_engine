#include <render_engine/resources/PushConstantsUpdater.h>

namespace RenderEngine
{
	void PushConstantsUpdater::update(size_t offset, std::span<const uint8_t> data)
	{
		vkCmdPushConstants(_command_buffer,
			_layout,
			_shader_stages,
			offset,
			data.size(),
			data.data());
	}
}