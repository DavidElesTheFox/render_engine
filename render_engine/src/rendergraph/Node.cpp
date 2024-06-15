#include <render_engine/rendergraph/Node.h>

#include <render_engine/CommandContext.h>
#include <render_engine/rendergraph/GraphVisitor.h>

#include <format>

#include <iostream>

namespace RenderEngine::RenderGraph
{
    void RenderNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void RenderNode::execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker)
    {
        const auto render_target_image_index = execution_context.getRenderTargetIndex();
        std::cout << "[WAT] Start rendering: " << getName() << " render target: " << render_target_image_index << std::endl;
        auto sync_object_holder = execution_context.getSyncObject(render_target_image_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());
        _renderer->draw(render_target_image_index);
        std::vector<VkCommandBufferSubmitInfo> command_buffer_infos;

        auto command_buffers = _renderer->getCommandBuffers(render_target_image_index);
        std::ranges::transform(command_buffers,
                               std::back_inserter(command_buffer_infos),
                               [](const auto& command_buffer)
                               {
                                   return VkCommandBufferSubmitInfo{
                                                       .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .commandBuffer = command_buffer,
                                                       .deviceMask = 0 };
                               });

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

        submit_info.commandBufferInfoCount = static_cast<uint32_t>(command_buffer_infos.size());
        submit_info.pCommandBufferInfos = command_buffer_infos.data();

        if (queue_tracker != nullptr)
        {
            queue_tracker->queueSubmit(std::move(submit_info),
                                       in_operations,
                                       *_command_context);
        }
        else
        {
            _command_context->queueSubmit(std::move(submit_info),
                                          in_operations,
                                          VK_NULL_HANDLE);
        }
        execution_context.setDrawCallRecorded(true);
        std::cout << "[WAT] Rendering finished: " << getName() << " render target: " << render_target_image_index << std::endl;

    }

    void TransferNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        const auto render_target_image_index = execution_context.getRenderTargetIndex();
        auto sync_object_holder = execution_context.getSyncObject(render_target_image_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());
        _scheduler.executeTasks(in_operations, _transfer_engine);
    }
    void TransferNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
    bool TransferNode::isActive() const
    {
        return _scheduler.hasAnyTask();
    }

    void ComputeNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        _compute_task->run(execution_context);

    }

    void ComputeNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void PresentNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        const auto render_target_image_index = execution_context.getRenderTargetIndex();

        std::cout << "[WAT] Start presenting: " << getName() << " render target: " << render_target_image_index << std::endl;
        auto sync_object_holder = execution_context.getSyncObject(render_target_image_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        VkSwapchainKHR swapChains[] = { _swap_chain.getDetails().swap_chain };
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapChains;

        present_info.pImageIndices = &render_target_image_index;
        _command_context->queuePresent(std::move(present_info), in_operations);
        std::cout << "[WAT] Presenting finished: " << getName() << std::endl;

    }

    void PresentNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void CpuNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        _cpu_task->run(execution_context);
    }
    void CpuNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
}