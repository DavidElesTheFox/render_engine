#pragma once
#include <volk.h>

#include <render_engine/DataTransferTasks.h>
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

    // TODO: Split into CoherentBuffer and Buffer class
    class Buffer
    {
    public:
        friend class ResourceStateMachine;

        Buffer(VkPhysicalDevice physical_device, LogicalDevice& logical_device, BufferInfo&& buffer_info);
        ~Buffer();

        void uploadMapped(std::span<const uint8_t> data_view);

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
        bool isMapped() const { return _buffer_info.mapped; }
        VkPhysicalDevice getPhysicalDevice() const { return _physical_device; }
        LogicalDevice& getLogicalDevice() const { return _logical_device; }

        void setInitialCommandContext(std::weak_ptr<CommandContext> command_context);

        void assignUploadTask(std::shared_ptr<UploadTask>);
        void assignDownloadTask(std::shared_ptr<DownloadTask>);

        std::shared_ptr<DownloadTask> clearDownloadTask();
        std::shared_ptr<UploadTask> getUploadTask() { return _ongoing_upload; }
    private:


        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice& _logical_device;
        VkBuffer _buffer{ VK_NULL_HANDLE };;
        VkDeviceMemory _buffer_memory{ VK_NULL_HANDLE };;
        BufferInfo _buffer_info;
        void* _mapped_memory{ nullptr };
        BufferState _buffer_state;
        std::shared_ptr<UploadTask> _ongoing_upload{ nullptr };
        std::shared_ptr<DownloadTask> _ongoing_download{ nullptr };
    };

    static_assert(IsResourceStateHolder_V<Buffer>, "Buffer must be a resource state holder");
}