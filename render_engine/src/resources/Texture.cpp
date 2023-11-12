#include <render_engine/resources/Texture.h>

#include <span>

namespace RenderEngine
{
	namespace
	{
		uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			throw std::runtime_error("failed to find suitable memory type!");
		}

		// TODO fixed values for now
		constexpr uint32_t kMipLevel = 1;
		constexpr uint32_t kArrayLayers = 1;
		constexpr uint32_t kLayerCount = 1;
		constexpr uint32_t kDepth = 1;
	}

	bool Texture::isImageCompatible(const Image& image) const
	{
		return image.getWidth() == _image.getWidth()
			&& image.getHeight() == _image.getHeight()
			&& image.getFormat() == _image.getFormat();
	}

	Texture::Texture(Image image,
		VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkImageAspectFlags aspect,
		VkShaderStageFlags shader_usage,
		std::set<uint32_t> compatible_queue_family_indexes)
		try : _physical_device(physical_device)
		, _logical_device(logical_device)
		, _staging_buffer(physical_device, logical_device, image.createBufferInfo())
		, _image(std::move(image))
		, _aspect(aspect)
		, _shader_usage(shader_usage)
		, _compatible_queue_family_indexes(std::move(compatible_queue_family_indexes))
	{
		std::vector<uint32_t> queue_family_indices{ _compatible_queue_family_indexes.begin(), _compatible_queue_family_indexes.end() };
		VkImageCreateInfo image_info{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.format = _image.getFormat();
		image_info.mipLevels = kMipLevel;
		image_info.arrayLayers = kArrayLayers;

		image_info.extent.depth = kDepth;
		image_info.extent.width = _image.getWidth();
		image_info.extent.height = _image.getHeight();

		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.sharingMode = queue_family_indices.size() > 1 
			? VK_SHARING_MODE_CONCURRENT
			: VK_SHARING_MODE_EXCLUSIVE; // due to upload queue
		image_info.samples = VK_SAMPLE_COUNT_1_BIT; 
		image_info.queueFamilyIndexCount = queue_family_indices.size();
		image_info.pQueueFamilyIndices = queue_family_indices.data();

		if (vkCreateImage(_logical_device, &image_info, nullptr, &_texture) != VK_SUCCESS) {
			throw std::runtime_error("Cannot create image");
		}

		VkMemoryRequirements memory_requirements{};
		vkGetImageMemoryRequirements(_logical_device, _texture, &memory_requirements);

		VkMemoryAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = memory_requirements.size;
		alloc_info.memoryTypeIndex = findMemoryType(_physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(_logical_device, &alloc_info, nullptr, &_texture_memory) != VK_SUCCESS)
		{
			throw std::runtime_error("Cannot allocate memory for image");
		}
		vkBindImageMemory(_logical_device, _texture, _texture_memory, 0);
	}
	catch (const std::exception&)
	{
		destroy();
	}


	TransferEngine::InFlightData Texture::upload(const Image& image, const SynchronizationPrimitives& synchronization_primitive, TransferEngine& transfer_engine, uint32_t dst_queue_family_index)
	{
		if (isImageCompatible(image) == false)
		{
			throw std::runtime_error("Input image is incompatible with the texture");
		}
		std::vector<uint8_t> image_data = image.readData();
		_staging_buffer.uploadMapped(std::span<uint8_t>(image_data.begin(), image_data.end()));
		auto result = transfer_engine.transfer(synchronization_primitive,
			[&](VkCommandBuffer command_buffer)
			{
				{
					VkImageMemoryBarrier2 barrier{};
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
					barrier.image = _texture;
					barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
					barrier.srcAccessMask = VK_ACCESS_2_NONE;
					barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.subresourceRange = createSubresourceRange();
					VkDependencyInfo dependency{};
					dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					dependency.imageMemoryBarrierCount = 1;
					dependency.pImageMemoryBarriers = &barrier;
					vkCmdPipelineBarrier2(command_buffer, &dependency);
				}
				{
					VkBufferImageCopy copy_region{};
					copy_region.bufferOffset = 0;
					copy_region.bufferRowLength = 0;
					copy_region.bufferImageHeight = 0;

					copy_region.imageSubresource.aspectMask = _aspect;
					copy_region.imageSubresource.mipLevel = 0;
					copy_region.imageSubresource.baseArrayLayer = 0;
					copy_region.imageSubresource.layerCount =1;

					copy_region.imageOffset = { 0, 0, 0 };
					copy_region.imageExtent = {
						.width = _image.getWidth(),
						.height = _image.getHeight(),
						.depth = kDepth
					};
					vkCmdCopyBufferToImage(command_buffer,
						_staging_buffer.getBuffer(),
						_texture,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, &copy_region);
				}

				{
					VkImageMemoryBarrier2 barrier{};
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
					barrier.image = _texture;
					barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					if (_shader_usage & VK_SHADER_STAGE_VERTEX_BIT)
					{
						barrier.dstStageMask |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
					}
					if (_shader_usage & VK_SHADER_STAGE_FRAGMENT_BIT)
					{
						barrier.dstStageMask |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					}
					barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
					barrier.srcQueueFamilyIndex = transfer_engine.getQueueFamilyIndex();
					barrier.dstQueueFamilyIndex = dst_queue_family_index;
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					barrier.subresourceRange = createSubresourceRange();
					VkDependencyInfo dependency{};
					dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					dependency.imageMemoryBarrierCount = 1;
					dependency.pImageMemoryBarriers = &barrier;
					vkCmdPipelineBarrier2(command_buffer, &dependency);
				}
			});
		return result;
	}

	VkImageSubresourceRange Texture::createSubresourceRange()
	{
		VkImageSubresourceRange result{};
		result.aspectMask = _aspect;
		result.baseMipLevel = 0;
		result.baseArrayLayer = 0;
		result.layerCount = kLayerCount;
		result.levelCount = 1;
		return result;
	}

	VkImageView Texture::createImageView(const ImageViewData& data)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = _texture;
		create_info.format = _image.getFormat();
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.subresourceRange.aspectMask = _aspect;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;

		VkImageView result;
		if (vkCreateImageView(_logical_device, &create_info, nullptr, &result) != VK_SUCCESS)
		{
			throw std::runtime_error("Cannot create image view");
		}
		return result;
	}

	VkSampler Texture::createSampler(const SamplerData& data)
	{
		VkSamplerCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		
		create_info.addressModeU = data.sampler_address_mode;
		create_info.addressModeV = data.sampler_address_mode;
		create_info.addressModeW = data.sampler_address_mode;

		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(_physical_device, &properties);
			create_info.anisotropyEnable = data.anisotroy_filter_enabled;
			create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		}
		create_info.borderColor = data.border_color;
		
		create_info.compareEnable = false;
		
		create_info.magFilter = data.mag_filter;
		create_info.minFilter = data.min_filter;

		create_info.unnormalizedCoordinates = data.unnormalize_coordinate;

		create_info.minLod = 0;
		create_info.maxLod = 0;
		create_info.mipLodBias = 0;
		create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		VkSampler result;
		if (vkCreateSampler(_logical_device, &create_info, nullptr, &result) != VK_SUCCESS)
		{
			throw std::runtime_error("Cannot create sampler");
		}

		return result;
	}

	void Texture::destroy() noexcept
	{
		vkFreeMemory(_logical_device, _texture_memory, nullptr);
		vkDestroyImage(_logical_device, _texture, nullptr);
	}

	[[nodiscar]]
	std::tuple<std::unique_ptr<Texture>, TransferEngine::InFlightData> TextureFactory::create(Image image,
		VkImageAspectFlags aspect,
		VkShaderStageFlags shader_usage,
		const SynchronizationPrimitives& synchronization_primitive,
		uint32_t dst_queue_family_index)
	{
		std::unique_ptr<Texture> result{ new Texture(image,
			_physical_device, _logical_device,
			aspect,
			shader_usage,
			_compatible_queue_family_indexes) };
		auto inflight_data = result->upload(image,
			synchronization_primitive,
			_transfer_engine,
			dst_queue_family_index);
		return { std::move(result), std::move(inflight_data) };
	}
}