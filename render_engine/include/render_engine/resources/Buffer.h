#pragma once
#include <volk.h>

#include <span>
#include <cstdint>
#include <memory>
namespace RenderEngine
{
	struct BufferInfo
	{
		VkBufferUsageFlags usage{ 0 };
		VkDeviceSize size{ 0 };
		VkMemoryPropertyFlags memory_properties{ 0 };
		bool mapped{ false };
	};

	class Buffer
	{
	public:
		Buffer(VkPhysicalDevice physical_device,
			VkDevice logical_device,
			BufferInfo&& buffer_info);
		~Buffer();

		void uploadMapped(std::span<const uint8_t> data_view);


		void uploadUnmapped(std::span<const uint8_t> data_view, VkQueue upload_queue, VkCommandPool command_pool);
		template<typename T>
		void uploadUnmapped(std::span<const T> data_view, VkQueue upload_queue, VkCommandPool command_pool)
		{
			uploadUnmapped(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data_view.data()), data_view.size() * sizeof(T)), upload_queue, command_pool);
		}
		VkBuffer getBuffer() { return _buffer; }
		VkDeviceSize getDeviceSize() const { return _buffer_info.size; }
	private:

		bool isMapped() const { return _buffer_info.mapped; }

		VkPhysicalDevice _physical_device;
		VkDevice _logical_device;
		VkBuffer _buffer;
		VkDeviceMemory _buffer_memory;
		BufferInfo _buffer_info;
		void* _mapped_memory{ nullptr };
	};
}