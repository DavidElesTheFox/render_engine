#pragma once

#include <volk.h>

#include <memory>

#include <render_engine/containers/BackBuffer.h>
#include <render_engine/resources/Buffer.h>

namespace RenderEngine
{

	class UniformBinding
	{
	public:
		struct FrameData
		{
			VkDescriptorSet _descriptor_set{ VK_NULL_HANDLE };
			std::unique_ptr<Buffer> _buffer{};
		};

		UniformBinding(VkDescriptorSetLayout descriptor_set_layout,
			BackBuffer<FrameData>&& back_buffer,
			VkDevice logical_device)
			:_logical_device(logical_device)
			, _back_buffer(std::move(back_buffer))
		{}
		~UniformBinding();
		UniformBinding(const UniformBinding&) = delete;
		UniformBinding(UniformBinding&&) = default;
		UniformBinding& operator=(const UniformBinding&) = delete;
		UniformBinding& operator=(UniformBinding&&) = default;

		Buffer& getBuffer(size_t frame_number)
		{
			return *_back_buffer[frame_number]._buffer;
		}
		VkDescriptorSet getDescriptorSet(size_t frame_number)
		{
			return _back_buffer[frame_number]._descriptor_set;
		}
	private:
		VkDevice _logical_device;
		BackBuffer<FrameData> _back_buffer;
	};
}
