#pragma once
#include <vulkan/vulkan.h>

#include <span>
#include <cstdint>
namespace RenderEngine
{
	class Buffer
	{
	public:
		Buffer(VkPhysicalDevice physical_device, VkDevice logical_device, VkBufferUsageFlags usage, VkDeviceSize size);
		~Buffer();

		void upload(std::span<const uint8_t> data_view, VkQueue upload_queue, VkCommandPool command_pool);
		template<typename T>
		void upload(std::span<const T> data_view, VkQueue upload_queue, VkCommandPool command_pool)
		{
			upload(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data_view.data()), data_view.size() * sizeof(T)), upload_queue, command_pool);
		}
		VkBuffer getBuffer() { return _buffer; }
		VkDeviceSize getDeviceSize() const { return _size; }
	private:
		VkPhysicalDevice _physical_device;
		VkDevice _logical_device;
		VkBuffer _buffer;
		VkDeviceMemory _buffer_memory;
		VkDeviceSize _size;
	};
}