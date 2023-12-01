#pragma once
#include <volk.h>

#include <render_engine/Device.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/resources/ResourceStateMachine.h>

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
		friend class ResourceStateMachine;

		Buffer(VkPhysicalDevice physical_device, VkDevice logical_device, BufferInfo&& buffer_info);
		~Buffer();

		void uploadMapped(std::span<const uint8_t> data_view);


		void uploadUnmapped(std::span<const uint8_t> data_view, TransferEngine& transfer_engine, uint32_t dst_queue_index);
		template<typename T>
		void uploadUnmapped(std::span<const T> data_view, TransferEngine& transfer_engine, uint32_t dst_queue_index)
		{
			uploadUnmapped(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data_view.data()), data_view.size() * sizeof(T)),
						   transfer_engine, dst_queue_index);
		}
		VkBuffer getBuffer() const { return _buffer; }
		VkDeviceSize getDeviceSize() const { return _buffer_info.size; }
		const ResourceStateMachine::BufferState& getResourceState() const
		{
			return _buffer_state;
		}
	private:

		bool isMapped() const { return _buffer_info.mapped; }
		void overrideResourceState(ResourceStateMachine::BufferState value)
		{
			_buffer_state = std::move(value);
		}

		VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
		VkDevice _logical_device{ VK_NULL_HANDLE };
		VkBuffer _buffer{ VK_NULL_HANDLE };;
		VkDeviceMemory _buffer_memory{ VK_NULL_HANDLE };;
		BufferInfo _buffer_info;
		void* _mapped_memory{ nullptr };
		ResourceStateMachine::BufferState _buffer_state;
	};
}