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
                               std::shared_ptr<SingleShotCommandContext>&& single_shot_command_context,
                               std::shared_ptr<CommandContext>&& command_context,
                               uint32_t back_buffer_count)
        : _device(device)
        , _gpu_resource_manager(device.getPhysicalDevice(), device.getLogicalDevice(), back_buffer_count, kMaxNumOfResources)
        , _command_context(std::move(command_context))
        , _transfer_engine(std::move(single_shot_command_context))
    {
        assert(_command_context != nullptr);
    }

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
            submit_tracker->queueSubmit(std::move(submit_info), sync_operations, *_command_context);
        }
        else
        {
            _command_context->queueSubmit(std::move(submit_info), sync_operations, VK_NULL_HANDLE);
        }
    }

    SingleShotCommandContext& RenderEngine::getTransferCommandContext()
    {
        return _transfer_engine.getTransferContext();
    }

}