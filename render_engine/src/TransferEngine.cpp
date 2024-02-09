#include <render_engine/TransferEngine.h>

namespace RenderEngine
{
    TransferEngine::TransferEngine(std::shared_ptr<CommandContext>&& transfer_context)
        : _transfer_context(std::move(transfer_context))
    {}

    void TransferEngine::transfer(const SyncOperations& sync_operations,
                                  std::function<void(VkCommandBuffer)> record_transfer_command)
    {

        VkCommandBuffer command_buffer = _transfer_context->createCommandBuffer(CommandContext::Usage::SingleSubmit);

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
        sync_operations.fillInfo(submitInfo);
        vkQueueSubmit2(_transfer_context->getQueue(), 1, &submitInfo, *sync_operations.getFence());
        vkWaitForFences(_transfer_context->getLogicalDevice(), 1, sync_operations.getFence(), VK_TRUE, UINT64_MAX);
    }
}