#include <render_engine/RenderContext.h>
#include <render_engine/TransferEngine.h>

#include <thread>
namespace RenderEngine
{
    TransferEngine::TransferEngine(CommandBufferContext transfer_context)
        : _transfer_context(std::move(transfer_context))
    {}

    void TransferEngine::transfer(const SyncOperations& sync_operations,
                                  std::function<void(VkCommandBuffer, SubmitScope* scope)> record_transfer_command,
                                  QueueSubmitTracker* queue_submit_tracker,
                                  SubmitScope&& current_scope)
    {
        uint32_t thread_index = RenderContext::context().getThreadingInfo().getThreadIndex(std::this_thread::get_id());
        VkCommandBuffer command_buffer = _transfer_context.getSingleShotFactory()->createCommandBuffer(thread_index);

        VkCommandBufferSubmitInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.commandBuffer = command_buffer;

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        _transfer_context.getQueue().getLogicalDevice()->vkBeginCommandBuffer(command_buffer, &begin_info);

        record_transfer_command(command_buffer, &current_scope);

        _transfer_context.getQueue().getLogicalDevice()->vkEndCommandBuffer(command_buffer);

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &command_buffer_info;
        if (queue_submit_tracker != nullptr)
        {
            queue_submit_tracker->queueSubmit(std::move(submitInfo), sync_operations, _transfer_context.getQueue(), std::move(current_scope));
        }
        else
        {
            _transfer_context.getQueue().queueSubmit(std::move(submitInfo), sync_operations, VK_NULL_HANDLE, std::move(current_scope));
        }
    }
}