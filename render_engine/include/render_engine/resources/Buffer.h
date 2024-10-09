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
    };

    class Buffer
    {
    public:
        friend class ResourceStateMachine;

        Buffer(VkPhysicalDevice physical_device, LogicalDevice& logical_device, BufferInfo&& buffer_info);
        ~Buffer();

        VkBuffer getBuffer() const { return _buffer; }
        VkDeviceSize getDeviceSize() const { return _buffer_info.size; }
        BufferState getResourceState(const SubmitScope& scope) const
        {
            auto it = std::ranges::find_if(_texture_states,
                                           [&](const auto& pair) { return pair.first.getId() == scope.getId(); });
            if (it == _texture_states.end())
            {
                BufferState result{};
                result.command_context = _global_queue_owner;
                return result;
            }
            else
            {
                return it->second;
            }
        }

        BufferState getResourceState(VkCommandBuffer command_buffer) const
        {
            using namespace std::views;
            auto it = std::ranges::find_if(_texture_states,
                                           [&](const auto& pair) { return pair.first.hasCommandBuffer(command_buffer); });

            if (it == _texture_states.end())
            {
                BufferState result{};
                result.command_context = _global_queue_owner;
                return result;
            }
            else
            {
                return it->second;
            }
        }
        void overrideResourceState(BufferState value, const SubmitScope& scope, ResourceAccessToken)
        {
            auto it = std::ranges::find_if(_texture_states,
                                           [&](const auto& pair) { return pair.first.getId() == scope.getId(); });
            _global_queue_owner = value.command_context;
            for (auto& [_, state] : _texture_states)
            {
                state.command_context = _global_queue_owner;
            }
            if (it == _texture_states.end())
            {
                _texture_states.push_back({ scope, value });
            }
            else
            {
                it->second = std::move(value);
            }
        }
        void removeResourceState(BufferState value, const SubmitScope& scope, ResourceAccessToken)
        {
            std::erase_if(_texture_states,
                          [&](const auto& pair) { return pair.first.getId() == scope.getId(); });
        }
        VkPhysicalDevice getPhysicalDevice() const { return _physical_device; }
        LogicalDevice& getLogicalDevice() const { return _logical_device; }

        void setInitialCommandContext(std::weak_ptr<SingleShotCommandBufferFactory> command_context);

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
        std::vector<std::pair<SubmitScope, BufferState>> _texture_states;
        std::weak_ptr<SingleShotCommandBufferFactory> _global_queue_owner;
        std::shared_ptr<UploadTask> _ongoing_upload{ nullptr };
        std::shared_ptr<DownloadTask> _ongoing_download{ nullptr };
    };

    static_assert(IsResourceStateHolder_V<Buffer>, "Buffer must be a resource state holder");

    class CoherentBuffer
    {
    public:
        friend class ResourceStateMachine;

        CoherentBuffer(VkPhysicalDevice physical_device, LogicalDevice& logical_device, BufferInfo&& buffer_info);
        ~CoherentBuffer();

        void upload(std::span<const uint8_t> data_view);
        template<typename T>
        void upload(std::span<const T> data)
        {
            upload(std::span(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T)));
        }
        template<typename T>
        void upload(std::span<T> data)
        {
            upload(std::span(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T)));
        }
        VkBuffer getBuffer() const { return _buffer; }
        VkDeviceSize getDeviceSize() const { return _buffer_info.size; }

        const void* getMemory() const { return _mapped_memory; }

        VkPhysicalDevice getPhysicalDevice() const { return _physical_device; }
        LogicalDevice& getLogicalDevice() const { return _logical_device; }

    private:
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice& _logical_device;
        VkBuffer _buffer{ VK_NULL_HANDLE };;
        VkDeviceMemory _buffer_memory{ VK_NULL_HANDLE };;
        BufferInfo _buffer_info;
        void* _mapped_memory{ nullptr };
    };
}