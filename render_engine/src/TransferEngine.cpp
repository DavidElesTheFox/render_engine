#include <render_engine/TransferEngine.h>

namespace RenderEngine
{
    TransferEngine::TransferEngine(VkDevice logical_device, uint32_t queue_familiy_index, VkQueue transfer_queue)
        : _logical_device(logical_device)
        , _queue_family_index(queue_familiy_index)
        , _transfer_queue(transfer_queue)
        , _command_pool_factory(logical_device, queue_familiy_index)
    {}

    TransferEngine::InFlightData TransferEngine::transfer(const SynchronizationPrimitives& synchronization_primitives,
                                                          std::function<void(VkCommandBuffer)> record_transfer_command,
                                                          VkQueue transfer_queue_override)
    {
        auto command_pool = _command_pool_factory.getCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = command_pool.command_pool;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(_logical_device, &alloc_info, &command_buffer);

        VkCommandBufferSubmitInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.commandBuffer = command_buffer;

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        record_transfer_command(command_buffer);

        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &command_buffer_info;

        submitInfo.signalSemaphoreInfoCount = synchronization_primitives.signal_semaphores.size();
        submitInfo.pSignalSemaphoreInfos = synchronization_primitives.signal_semaphores.data();

        submitInfo.waitSemaphoreInfoCount = synchronization_primitives.wait_semaphores.size();
        submitInfo.pWaitSemaphoreInfos = synchronization_primitives.wait_semaphores.data();

        vkQueueSubmit2(transfer_queue_override, 1, &submitInfo, synchronization_primitives.on_finished_fence);

        return { command_buffer, std::move(command_pool),_logical_device };
    }
    TransferEngine::InFlightData::~InFlightData()
    {
        destroy();
    }
    void TransferEngine::InFlightData::destroy() noexcept
    {
        if (_logical_device == VK_NULL_HANDLE)
        {
            return;
        }
        vkFreeCommandBuffers(_logical_device, _command_pool.command_pool, 1, &_command_buffer);
        vkDestroyCommandPool(_logical_device, _command_pool.command_pool, nullptr);
        _command_pool = {};
        _command_buffer = VK_NULL_HANDLE;
        _logical_device = VK_NULL_HANDLE;
    }
}