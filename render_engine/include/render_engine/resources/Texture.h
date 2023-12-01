#pragma once

#include <volk.h>

#include <render_engine/assets/Image.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/SynchronizationPrimitives.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/resources/ResourceStateMachine.h>

#include <set>

namespace RenderEngine
{
	class Texture
	{
	public:
		friend class ResourceStateMachine;
		friend class TextureFactory;
		struct ImageViewData
		{
		};

		struct SamplerData
		{
			VkFilter mag_filter{ VK_FILTER_LINEAR };
			VkFilter min_filter{ VK_FILTER_LINEAR };
			VkSamplerAddressMode sampler_address_mode{ VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT };
			bool anisotroy_filter_enabled{ false };
			VkBorderColor border_color{ VK_BORDER_COLOR_INT_OPAQUE_BLACK };
			bool unnormalize_coordinate{ false };
		};
		
		
		~Texture()
		{
			destroy();
		}
		bool isImageCompatible(const Image& image) const;

		[[nodiscard]]
		TransferEngine::InFlightData upload(const Image& image, 
			const SynchronizationPrimitives& synchronization_primitive, 
			TransferEngine& transfer_engine, 
			uint32_t dst_queue_family_index);

		VkImageView createImageView(const ImageViewData& data);
		VkSampler createSampler(const SamplerData& data);

		VkImageSubresourceRange createSubresourceRange() const;
		VkImage getVkImage() const { return _texture; }

		const ResourceStateMachine::TextureState& getResourceState() const
		{
			return _texture_state;
		}

	private:
		Texture(Image image,
			VkPhysicalDevice physical_device,
			VkDevice logical_device, 
			VkImageAspectFlags aspect,
			VkShaderStageFlags shader_usage,
			std::set<uint32_t> compatible_queue_family_indexes);
		void destroy() noexcept;

		void overrideResourceState(ResourceStateMachine::TextureState value)
		{
			_texture_state = std::move(value);
		}
		VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
		VkDevice _logical_device{ VK_NULL_HANDLE };
		VkImage _texture{ VK_NULL_HANDLE };
		Image _image;
		VkDeviceMemory _texture_memory{ VK_NULL_HANDLE };
		Buffer _staging_buffer;
		VkImageAspectFlags _aspect{ VK_IMAGE_ASPECT_NONE };
		VkShaderStageFlags _shader_usage{ VK_SHADER_STAGE_ALL };
		std::set<uint32_t> _compatible_queue_family_indexes;
		ResourceStateMachine::TextureState _texture_state;
	};

	class TextureView
	{
	public:
		TextureView(Texture& texture, 
			VkImageView image_view, 
			VkSampler sampler,
			VkPhysicalDevice physical_device, 
			VkDevice logical_device)
			: _texture(texture)
			, _image_view(image_view)
			, _sampler(sampler)
			, _logical_device(logical_device)
		{}
		~TextureView()
		{
			vkDestroyImageView(_logical_device, _image_view, nullptr);
			vkDestroySampler(_logical_device, _sampler, nullptr);
		}

		Texture& getTexture() { return _texture; }
		VkImageView getImageView() { return _image_view; }
		VkSampler getSamler() { return _sampler; }
	private:
		Texture& _texture;
		VkImageView _image_view{ VK_NULL_HANDLE };
		VkSampler _sampler{ VK_NULL_HANDLE };
		VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
		VkDevice _logical_device{ VK_NULL_HANDLE };
	};

	class TextureFactory
	{
	public:
		TextureFactory(TransferEngine& transfer_engine,
			std::set<uint32_t> compatible_queue_family_indexes,
			VkPhysicalDevice physical_device,
			VkDevice logical_device)
			: _transfer_engine(transfer_engine)
			, _compatible_queue_family_indexes(std::move(compatible_queue_family_indexes))
			, _physical_device(physical_device)
			, _logical_device(logical_device)
		{}

		[[nodiscar]]
		std::tuple<std::unique_ptr<Texture>, TransferEngine::InFlightData> create(Image image,
			VkImageAspectFlags aspect,
			VkShaderStageFlags shader_usage,
			const SynchronizationPrimitives& synchronization_primitive,
			uint32_t dst_queue_family_index);
	private:
		TransferEngine& _transfer_engine;
		std::set<uint32_t> _compatible_queue_family_indexes;
		VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
		VkDevice _logical_device{ VK_NULL_HANDLE };

	};
}