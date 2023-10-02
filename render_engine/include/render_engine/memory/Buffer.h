#pragma once
#include <volk.h>

#include <span>
#include <cstdint>
#include <memory>
namespace RenderEngine
{
	class Buffer
	{
	public:
		static std::unique_ptr<Buffer> CreateAttributeBuffer(VkPhysicalDevice physical_device,
			VkDevice logical_device,
			VkBufferUsageFlags usage,
			VkDeviceSize size);
		static std::unique_ptr<Buffer> CreateUniformBuffer(VkPhysicalDevice physical_device,
			VkDevice logical_device,
			VkDeviceSize size);
		~Buffer();

		void uploadMapped(std::span<const uint8_t> data_view);


		void uploadUnmapped(std::span<const uint8_t> data_view, VkQueue upload_queue, VkCommandPool command_pool);
		template<typename T>
		void uploadUnmapped(std::span<const T> data_view, VkQueue upload_queue, VkCommandPool command_pool)
		{
			uploadUnmapped(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data_view.data()), data_view.size() * sizeof(T)), upload_queue, command_pool);
		}
		VkBuffer getBuffer() { return _buffer; }
		VkDeviceSize getDeviceSize() const { return _size; }
	private:
		Buffer(VkPhysicalDevice physical_device,
			VkDevice logical_device,
			VkBufferUsageFlags usage,
			VkDeviceSize size,
			VkMemoryPropertyFlags properties);
		bool isMapped() const { return _mapped_memory != nullptr; }

		VkPhysicalDevice _physical_device;
		VkDevice _logical_device;
		VkBuffer _buffer;
		VkDeviceMemory _buffer_memory;
		VkDeviceSize _size;
		void* _mapped_memory{ nullptr };
	};
}