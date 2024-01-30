#pragma once

#include <render_engine/CommandPoolFactory.h>
#include <render_engine/Device.h>
#include <render_engine/synchronization/SyncOperations.h>

#include <functional>
namespace RenderEngine
{
    class TransferEngine
    {
    public:
        class InFlightData
        {
            friend class TransferEngine;
            InFlightData(VkCommandBuffer command_buffer,
                         CommandPoolFactory::CommandPool command_pool,
                         VkDevice logical_device)
                : _command_buffer(command_buffer)
                , _command_pool(std::move(command_pool))
                , _logical_device(logical_device)
            {}
        public:
            InFlightData(InFlightData&& o) noexcept
            {
                std::swap(_command_buffer, o._command_buffer);
                std::swap(_command_pool, o._command_pool);
                std::swap(_logical_device, o._logical_device);
            }
            InFlightData(const InFlightData&) = delete;

            InFlightData& operator=(InFlightData&& o) noexcept
            {
                destroy();
                std::swap(_command_buffer, o._command_buffer);
                std::swap(_command_pool, o._command_pool);
                std::swap(_logical_device, o._logical_device);
                return *this;
            }
            InFlightData& operator=(const InFlightData&) = delete;

            ~InFlightData();
            void destroy() noexcept;
        private:
            VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
            CommandPoolFactory::CommandPool _command_pool;
            VkDevice _logical_device{ VK_NULL_HANDLE };
        };

        TransferEngine(VkDevice logical_device,
                       uint32_t queue_familiy_index,
                       VkQueue transfer_queue);

        [[nodiscard]]
        InFlightData transfer(const SyncOperations& sync_operations,
                              std::function<void(VkCommandBuffer)> record_transfer_command)
        {
            return transfer(sync_operations, record_transfer_command, _transfer_queue);
        }
        [[nodiscard]]
        InFlightData transfer(const SyncOperations& sync_operations,
                              std::function<void(VkCommandBuffer)> record_transfer_command,
                              VkQueue transfer_queue_override);

        uint32_t getQueueFamilyIndex() const { return _queue_family_index; }
    private:
        VkDevice _logical_device{ VK_NULL_HANDLE };
        uint32_t _queue_family_index{ 0 };
        VkQueue _transfer_queue{ VK_NULL_HANDLE };
        CommandPoolFactory _command_pool_factory;
    };
}