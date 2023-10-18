#pragma once

#include <volk.h>
#include <span>

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

		void update(VkShaderStageFlags shader_stages, size_t offset, std::span<const uint8_t> data);
	private:
		VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
		VkPipelineLayout _layout{ VK_NULL_HANDLE };	};
}