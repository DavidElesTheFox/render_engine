#pragma once

#include <render_engine/Device.h>
#include <render_engine/CommandPoolFactory.h>
#include <render_engine/SynchronizationPrimitives.h>

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
			InFlightData(InFlightData&&) = default;
			InFlightData(const InFlightData&) = delete;

			InFlightData operator=(InFlightData&&) = delete;
			InFlightData operator=(const InFlightData&) = delete;

			~InFlightData();
			void destroy() noexcept;
		private:
			VkCommandBuffer _command_buffer;
			CommandPoolFactory::CommandPool _command_pool;
			VkDevice _logical_device;
		};

		TransferEngine(Device& device,
			uint32_t queue_familiy_index,
			VkQueue transfer_queue);

		[[nodiscard]]
		InFlightData transfer(const SynchronizationPrimitives& synchronization_primitives,
			std::function<void(VkCommandBuffer)> record_transfer_command)
		{
			return transfer(synchronization_primitives, record_transfer_command, _transfer_queue);
		}
		InFlightData transfer(const SynchronizationPrimitives& synchronization_primitives,
			std::function<void(VkCommandBuffer)> record_transfer_command,
			VkQueue transfer_queue_override);
		uint32_t getQueueFamilyIndex() const { return _queue_family_index; }
	private:
		Device& _device;
		uint32_t _queue_family_index{ 0 };
		VkQueue _transfer_queue{ VK_NULL_HANDLE };
		CommandPoolFactory _command_pool_factory;
	};
}