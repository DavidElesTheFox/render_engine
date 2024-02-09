#include <render_engine/RenderEngine.h>

#include <render_engine/RenderContext.h>
#include <render_engine/RendererFactory.h>

namespace RenderEngine
{
    namespace
    {
        constexpr uint32_t kMaxNumOfResources = 10;
    }
    RenderEngine::RenderEngine(Device& device, std::shared_ptr<CommandContext>&& command_context, size_t back_buffer_count)
        : _device(device)
        , _gpu_resource_manager(device.getPhysicalDevice(), device.getLogicalDevice(), back_buffer_count, kMaxNumOfResources)
        , _command_context(command_context->clone())
        , _transfer_engine(std::move(command_context))
    {

    }

    void RenderEngine::submitDrawCalls(const std::vector<VkCommandBufferSubmitInfo>& command_buffers, const SyncOperations& sync_operations)
    {
        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

        submit_info.commandBufferInfoCount = command_buffers.size();
        submit_info.pCommandBufferInfos = command_buffers.data();

        sync_operations.fillInfo(submit_info);
        if (vkQueueSubmit2(_command_context->getQueue(), 1, &submit_info, *sync_operations.getFence()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

}