#include <render_engine/rendergraph/Node.h>

#include <render_engine/CommandContext.h>
#include <render_engine/rendergraph/GraphVisitor.h>

#include <format>

namespace RenderEngine::RenderGraph
{
    void RenderNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
    std::unique_ptr<Job> RenderNode::createJob(const SyncObject& in_operation_group)
    {
        auto callback = [&](Job::ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker)
            {
                const auto render_target_image_index = execution_context.getRenderTargetIndex();
                const auto& in_operations = in_operation_group.getOperationsGroup(Link::syncGroup(render_target_image_index));
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
            };
        std::unique_ptr<QueueSubmitTracker> tracker;
        return std::make_unique<Job>(getName(), std::move(callback), std::move(tracker));
    }
    std::unique_ptr<Job> TransferNode::createJob(const SyncObject& in_operation_group)
    {
        auto callback = [&](Job::ExecutionContext& execution_context, QueueSubmitTracker*)
            {
                const auto render_target_image_index = execution_context.getRenderTargetIndex();
                const auto& in_operations = in_operation_group.getOperationsGroup(Link::syncGroup(render_target_image_index));
                _scheduler.executeTasks(in_operations, _transfer_engine);
            };
        return std::make_unique<Job>(getName(), std::move(callback));
    }
    void TransferNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
    bool TransferNode::isActive() const
    {
        return _scheduler.hasAnyTask();
    }

    std::unique_ptr<Job> ComputeNode::createJob(const SyncObject& in_operation_group)
    {
        auto callback = [&](Job::ExecutionContext& execution_context, QueueSubmitTracker*)
            {
                _compute_task->run(in_operation_group, execution_context);
            };
        return std::make_unique<Job>(getName(), std::move(callback));

    }

    void ComputeNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    std::unique_ptr<Job> PresentNode::createJob(const SyncObject& in_operation_group)
    {
        auto callback = [&](Job::ExecutionContext& execution_context, QueueSubmitTracker*)
            {
                const auto render_target_image_index = execution_context.getRenderTargetIndex();
                const auto& in_operations = in_operation_group.getOperationsGroup(Link::syncGroup(render_target_image_index));

                VkPresentInfoKHR present_info{};
                present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                VkSwapchainKHR swapChains[] = { _swap_chain.getDetails().swap_chain };
                present_info.swapchainCount = 1;
                present_info.pSwapchains = swapChains;

                present_info.pImageIndices = &render_target_image_index;
                _command_context->queuePresent(std::move(present_info), in_operations);
            };
        return std::make_unique<Job>(getName(), std::move(callback));

    }

    void PresentNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    std::unique_ptr<Job> CpuNode::createJob(const SyncObject& in_operations)
    {
        auto callback = [&](Job::ExecutionContext& execution_context, QueueSubmitTracker*)
            {
                _cpu_task->run(in_operations, execution_context);
            };
        return std::make_unique<Job>(getName(), std::move(callback));
    }
    void CpuNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
}