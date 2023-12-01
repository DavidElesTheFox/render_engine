#pragma once

#include <optional>
#include <unordered_map>

#include <volk.h>


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
			std::optional<uint32_t> queue_family_index{ 0 };

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
			TextureState&& setQueueFamilyIndex(uint32_t queue_family_index)
			{
				this->queue_family_index = queue_family_index;
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
					&& queue_family_index == o.queue_family_index;
			}
			bool operator!=(const TextureState& o) const
			{
				return ((*this) == o) == false;
			}
		};

		struct BufferState
		{
			VkPipelineStageFlagBits2 pipeline_stage{ VK_PIPELINE_STAGE_2_NONE };
			VkAccessFlags2 access_flag{ VK_ACCESS_2_NONE };
			std::optional<uint32_t> queue_family_index{ 0 };

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

			BufferState&& setQueueFamilyIndex(uint32_t queue_family_index)&&
			{
				this->queue_family_index = queue_family_index;
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
					&& queue_family_index == o.queue_family_index;
			}
			bool operator!=(const BufferState& o) const
			{
				return ((*this) == o) == false;
			}
		};
		ResourceStateMachine() = default;


		void recordStateChange(Texture* texture, TextureState next_state);
		void recordStateChange(Buffer* buffer, BufferState next_state);

		void commitChanges(VkCommandBuffer command_buffer);
	private:
		std::vector<VkImageMemoryBarrier2> createImageBarriers();
		std::vector<VkBufferMemoryBarrier2> createBufferBarriers();

		std::unordered_map<Texture*, std::optional<TextureState>> _images{};
		std::unordered_map<Buffer*, std::optional<BufferState>> _buffers{};
	};
}