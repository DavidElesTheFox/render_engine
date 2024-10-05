#include <render_engine/RenderEngine.h>

#include <render_engine/RenderContext.h>
#include <render_engine/RendererFactory.h>

#include <cassert>

namespace RenderEngine
{
    namespace
    {
        constexpr uint32_t kMaxNumOfResources = 10;
    }
    RenderEngine::RenderEngine(Device& device,
                               CommandBufferContext command_buffer_context,
                               TransferEngine transfer_engine,
                               TransferEngine transfer_engine_on_render_queue,
                               uint32_t back_buffer_count)
        : _device(device)
        , _gpu_resource_manager(device.getPhysicalDevice(), device.getLogicalDevice(), back_buffer_count, kMaxNumOfResources)
        , _command_buffer_context(std::move(command_buffer_context))
        , _transfer_engine(transfer_engine)
        , _transfer_engine_on_render_queue(transfer_engine_on_render_queue)
    {}

    void RenderEngine::submitDrawCalls(const std::vector<VkCommandBufferSubmitInfo>& command_buffers,
                                       const SyncOperations& sync_operations,
                                       QueueSubmitTracker* submit_tracker)
    {
        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

        submit_info.commandBufferInfoCount = static_cast<uint32_t>(command_buffers.size());
        submit_info.pCommandBufferInfos = command_buffers.data();

        if (submit_tracker != nullptr)
        {
            submit_tracker->queueSubmit(std::move(submit_info), sync_operations, _command_buffer_context.getQueue());
        }
        else
        {
            _command_buffer_context.getQueue().queueSubmit(std::move(submit_info), sync_operations, VK_NULL_HANDLE);
        }
    }


}