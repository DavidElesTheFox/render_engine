#pragma once

#include <volk.h>
#include <span>

namespace RenderEngine
{
	class PushConstantsUpdater
	{
	public:
		PushConstantsUpdater(VkCommandBuffer command_buffer,
			VkPipelineLayout layout,
			VkShaderStageFlags shader_stages)
			: _command_buffer(command_buffer)
			, _layout(layout)
			, _shader_stages(shader_stages)
		{}

		void update(size_t offset, std::span<const uint8_t> data);
	private:
		VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
		VkPipelineLayout _layout{ VK_NULL_HANDLE };
		VkShaderStageFlags _shader_stages{ 0 };
	};
}