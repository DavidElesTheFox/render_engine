#pragma once
#include <volk.h>

#include <render_engine/DataTransferTasks.h>
#include <render_engine/Device.h>
#include <render_engine/LogicalDevice.h>
#include <render_engine/synchronization/ResourceStateMachine.h>
#include <render_engine/synchronization/ResourceStates.h>
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
        struct PreservedState
        {
            PreservedState() = default;

            explicit PreservedState(const BufferState& buffer_state)
                : command_context(buffer_state.command_context)
            {}

            void reset(const BufferState& buffer_state)
            {
                command_context = buffer_state.command_context;
            }

            explicit operator BufferState() const
            {
                BufferState result;
                result.command_context = command_context;
                return result;
            }
            std::optional<uint32_t> getQueueFamilyIndex() const
            {
                return command_context.expired() ? std::nullopt : std::optional{ command_context.lock()->getQueue().getQueueFamilyIndex() };
            }
            std::weak_ptr<SingleShotCommandBufferFactory> command_context;
        };
        friend class ResourceStateMachine;

        Buffer(VkPhysicalDevice physical_device, LogicalDevice& logical_device, BufferInfo&& buffer_info);
        ~Buffer();

        VkBuffer getBuffer() const { return _buffer; }
        VkDeviceSize getDeviceSize() const { return _buffer_info.size; }

        const PreservedState& getGlobalResourceState() const { return _preserved_state; }
        BufferState getResourceState(SubmitScope* scope)
        {
            auto it = std::ranges::find_if(_buffer_states,
                                           [&](const auto& pair) { return pair.first == scope; });
            if (it == _buffer_states.end())
            {
                std::lock_guard lock(_preserve_state_mutex);
                _buffer_states.push_back({ scope, static_cast<BufferState>(_preserved_state) });
                it = _buffer_states.end() - 1;
                scope->addCleanUpFunction([this](const SubmitScope* scope) { removeResourceState(scope, ResourceAccessToken{}); });
            }
            return it->second;
        }

        void removeResourceState(const SubmitScope* scope, ResourceAccessToken)
        {
            std::lock_guard lock(_preserve_state_mutex);
            auto it = std::ranges::find_if(_buffer_states,
                                           [&](const auto& pair) { return pair.first == scope; });
            if (it != _buffer_states.end())
            {
                _preserved_state.reset(it->second);

                _buffer_states.erase(it);
            }
        }

        void overrideResourceState(BufferState value, SubmitScope* scope, ResourceAccessToken)
        {
            auto it = std::ranges::find_if(_buffer_states,
                                           [&](const auto& pair) { return pair.first == scope; });
#ifdef RENDER_ENGINE_DEBUG
            {
                auto debug_it = std::ranges::find_if(_buffer_states,
                                                     [&](const auto& pair) { return (pair.second.dirty_flags & value.dirty_flags) != 0; });
                if (debug_it != _buffer_states.end())
                {
                    auto& debugger = RenderContext::context().getDebugger();
                    debugger.print(Debug::Topics::ResourceStateValidation{},
                                   "Warning: State change cross reference detected. This Texture has an active submit scope (it is still recording commands into the queue),"
                                   " and that commands are modifying the same state of the texture what the current commands also modified. The other scope id is: {:d} the current: {:d}."
                                   " This is a Warning message, if between the two submit there are proper synchronization then it is not an issue.",
                                   debug_it->first->getId(),
                                   scope->getId());
                }
            }
#endif
            if (it == _buffer_states.end())
            {
                _buffer_states.push_back({ scope, value });
                scope->addCleanUpFunction([this](const SubmitScope* scope) { removeResourceState(scope, ResourceAccessToken{}); });
            }
            else
            {
                it->second = std::move(value);
            }
        }
        VkPhysicalDevice getPhysicalDevice() const { return _physical_device; }
        LogicalDevice& getLogicalDevice() const { return _logical_device; }

        void setInitialCommandContext(std::weak_ptr<SingleShotCommandBufferFactory> command_context)
        {
            std::lock_guard lock(_preserve_state_mutex);
            assert(_preserved_state.command_context.expired());
            _preserved_state.command_context = command_context;
        }

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
        std::vector<std::pair<const SubmitScope*, BufferState>> _buffer_states;
        PreservedState _preserved_state;
        std::mutex _preserve_state_mutex;
        std::shared_ptr<UploadTask> _ongoing_upload{ nullptr };
        std::shared_ptr<DownloadTask> _ongoing_download{ nullptr };
    };

    static_assert(ResourceStateHolder<Buffer>, "Buffer must be a resource state holder");

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