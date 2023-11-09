#pragma once

#include <volk.h>

#include <render_engine/assets/Image.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/SynchronizationPrimitives.h>
#include <render_engine/TransferEngine.h>
namespace RenderEngine
{
	class Texture
	{
	public:
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

		[[nodiscar]]
		static std::tuple<std::unique_ptr<Texture>, TransferEngine::InFlightData> Create(Image image, 
			VkImageAspectFlags aspect,
			VkShaderStageFlags shader_usage,
			const SynchronizationPrimitives& synchronization_primitive,
			TransferEngine& transfer_engine,
			uint32_t dst_queue_family_index,
			VkPhysicalDevice physical_device,
			VkDevice logical_device)
		{
			std::unique_ptr<Texture> result{ new Texture(image, physical_device, logical_device, aspect, shader_usage) };
			auto inflight_data = result->upload(image, synchronization_primitive, transfer_engine, dst_queue_family_index);
			return { std::move(result), std::move(inflight_data) };
		}
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

	private:
		Texture(Image image,
			VkPhysicalDevice physical_device,
			VkDevice logical_device, 
			VkImageAspectFlags aspect,
			VkShaderStageFlags shader_usage);
		void destroy() noexcept;
		VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
		VkDevice _logical_device{ VK_NULL_HANDLE };
		VkImage _texture{ VK_NULL_HANDLE };
		Image _image;
		VkDeviceMemory _texture_memory{ VK_NULL_HANDLE };
		Buffer _staging_buffer;
		VkImageAspectFlags _aspect{ VK_IMAGE_ASPECT_NONE };
		VkShaderStageFlags _shader_usage;
	};
}