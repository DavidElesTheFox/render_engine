#include <render_engine/Buffer.h>

#include <map>
#include <stdexcept>
#include <string>
#include <cassert>
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
	std::pair<VkBuffer, VkDeviceMemory> createBuffer(
		VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties)
	{
		VkBuffer buffer;
		VkDeviceMemory buffer_memory;
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logical_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements mem_requirements;
		vkGetBufferMemoryRequirements(logical_device, buffer, &mem_requirements);

		VkMemoryAllocateInfo allocInfo{};

		try
		{
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = mem_requirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties);

			if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &buffer_memory) != VK_SUCCESS) {
				vkDestroyBuffer(logical_device, buffer, nullptr);
				throw std::runtime_error("failed to allocate buffer memory!");
			}

		}
		catch (const std::runtime_error&)
		{
			vkDestroyBuffer(logical_device, buffer, nullptr);
			throw;
		}

		vkBindBufferMemory(logical_device, buffer, buffer_memory, 0);
		return { buffer, buffer_memory };
	}

	void copyBuffer(VkDevice logical_device, VkQueue upload_queue, VkCommandPool upload_pool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = upload_pool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(logical_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(upload_queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(upload_queue);

		vkFreeCommandBuffers(logical_device, upload_pool, 1, &commandBuffer);
	}

}

namespace RenderEngine
{
	std::unique_ptr<Buffer> Buffer::CreateAttributeBuffer(VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkBufferUsageFlags usage,
		VkDeviceSize size)
	{
		return std::unique_ptr<Buffer>(new Buffer(physical_device,
			logical_device,
			usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			size,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	}

	std::unique_ptr<Buffer> Buffer::CreateUniformBuffer(VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkDeviceSize size)
	{
		auto result = std::unique_ptr<Buffer>(new Buffer(physical_device,
			logical_device,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			size,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		vkMapMemory(logical_device, result->_buffer_memory, 0, size, 0, &result->_mapped_memory);
		return result;
	}


	Buffer::Buffer(VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkBufferUsageFlags usage,
		VkDeviceSize size,
		VkMemoryPropertyFlags properties)
		: _physical_device(physical_device)
		, _logical_device(logical_device)
		, _size(size)
	{
		std::tie(_buffer, _buffer_memory) = createBuffer(physical_device,
			logical_device,
			size,
			usage,
			properties);
	}
	Buffer::~Buffer()
	{
		vkDestroyBuffer(_logical_device, _buffer, nullptr);
		vkFreeMemory(_logical_device, _buffer_memory, nullptr);
	}

	void Buffer::uploadUnmapped(std::span<const uint8_t> data_view, VkQueue upload_queue, VkCommandPool command_pool)
	{
		assert(isMapped() == false);
		if (_size != data_view.size())
		{
			throw std::runtime_error("Invalid size during upload. Buffer size: " + std::to_string(_size) + " data to upload: " + std::to_string(data_view.size()));
		}
		auto [staging_buffer, staging_memory] = createBuffer(_physical_device,
			_logical_device,
			_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data = nullptr;
		vkMapMemory(_logical_device, staging_memory, 0, _size, 0, &data);
		memcpy(data, data_view.data(), (size_t)_size);
		vkUnmapMemory(_logical_device, staging_memory);

		copyBuffer(_logical_device, upload_queue, command_pool, staging_buffer, _buffer, _size);
		vkDestroyBuffer(_logical_device, staging_buffer, nullptr);
		vkFreeMemory(_logical_device, staging_memory, nullptr);
	}

	void Buffer::uploadMapped(std::span<const uint8_t> data_view)
	{
		assert(isMapped());
		memcpy(_mapped_memory, data_view.data(), data_view.size());
	}

}