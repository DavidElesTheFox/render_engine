#pragma once
#include <volk.h>

#include <render_engine/Device.h>
#include <render_engine/LogicalDevice.h>
#include <render_engine/synchronization/ResourceStateMachine.h>
#include <render_engine/TransferEngine.h>

#include <cstdint>
#include <memory>
#include <span>
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

        Buffer(VkPhysicalDevice physical_device, LogicalDevice& logical_device, BufferInfo&& buffer_info);
        ~Buffer();

        void uploadMapped(std::span<const uint8_t> data_view);

        void uploadUnmapped(std::span<const uint8_t> data_view, TransferEngine& transfer_engine, CommandContext* dst_context, SyncOperations sync_operations);
        template<typename T>
        void uploadUnmapped(std::span<const T> data_view, TransferEngine& transfer_engine, CommandContext* dst_context, SyncOperations sync_operations)
        {
            uploadUnmapped(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data_view.data()), data_view.size() * sizeof(T)),
                           transfer_engine, dst_context, sync_operations);
        }
        VkBuffer getBuffer() const { return _buffer; }
        VkDeviceSize getDeviceSize() const { return _buffer_info.size; }
        const BufferState& getResourceState() const
        {
            return _buffer_state;
        }
        const void* getMemory() const { return _mapped_memory; }
        void overrideResourceState(BufferState value, ResourceAccessToken)
        {
            _buffer_state = std::move(value);
        }

    private:

        bool isMapped() const { return _buffer_info.mapped; }

        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice& _logical_device;
        VkBuffer _buffer{ VK_NULL_HANDLE };;
        VkDeviceMemory _buffer_memory{ VK_NULL_HANDLE };;
        BufferInfo _buffer_info;
        void* _mapped_memory{ nullptr };
        BufferState _buffer_state;
    };

    static_assert(IsResourceStateHolder_V<Buffer>, "Buffer must be a resource state holder");
}