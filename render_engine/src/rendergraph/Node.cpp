#include <render_engine/rendergraph/Node.h>

#include <render_engine/CommandContext.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/RenderContext.h>
#include <render_engine/rendergraph/GraphVisitor.h>
#include <render_engine/rendergraph/Topic.h>

#include <format>
#include <iostream>


namespace RenderEngine::RenderGraph
{
    namespace
    {
        static thread_local const std::string g_thread_name = std::format("Thread-{:}", std::this_thread::get_id());
    }
#define PROFILE_NODE() PROFILE_THREAD(g_thread_name.c_str()); PROFILE_SCOPE()

    void RenderNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void RenderNode::execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker)
    {
        PROFILE_NODE();
        const auto pool_index = execution_context.getPoolIndex();
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start rendering: {:s} render target: {:d} (sync index: {:d}) [thread:{}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());
        auto sync_object_holder = execution_context.getSyncObject(pool_index.sync_object_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());
        _renderer->draw(pool_index.render_target_index);
        std::vector<VkCommandBufferSubmitInfo> command_buffer_infos;

        auto command_buffers = _renderer->getCommandBuffers(pool_index.render_target_index);
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

        if (isUsesTracking() && queue_tracker != nullptr)
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
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Rendering finished: {:s} render target: {:d} (sync index: {:d}) [thread: {}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());

    }

    void TransferNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        PROFILE_NODE();

        const auto pool_index = execution_context.getPoolIndex();
        auto sync_object_holder = execution_context.getSyncObject(pool_index.sync_object_index);
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

    void DeviceSynchronizeNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        const auto pool_index = execution_context.getPoolIndex();
        auto sync_object_holder = execution_context.getSyncObject(pool_index.sync_object_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());
        _device->synchronizeStagingArea(in_operations);
    }
    void DeviceSynchronizeNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
    bool DeviceSynchronizeNode::isActive() const
    {
        return _device->getDataTransferContext().getScheduler().hasAnyTask();
    }


    void ComputeNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        PROFILE_NODE();
        _compute_task->run(execution_context);
    }

    void ComputeNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void PresentNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        PROFILE_NODE();

        const auto pool_index = execution_context.getPoolIndex();
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start presenting: {:s} render target: {:d} (sync index: {:d}) [thread: {}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());
        auto sync_object_holder = execution_context.getSyncObject(pool_index.sync_object_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());
        {
            PROFILE_SCOPE("PresentNode::execute - Accessing swap chain");

            VkPresentInfoKHR present_info{};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            present_info.pImageIndices = &pool_index.render_target_index;
            _command_context->queuePresent(std::move(present_info), in_operations, _swap_chain);
            RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                         "Presenting finished: {:s} render target: {:d} (sync index: {:d}) [thread: {}]",
                                                         getName(),
                                                         pool_index.render_target_index,
                                                         pool_index.sync_object_index,
                                                         std::this_thread::get_id());
        }
    }

    void PresentNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void CpuNode::execute(ExecutionContext& execution_context, QueueSubmitTracker*)
    {
        PROFILE_NODE();
        _cpu_task->run(execution_context);
    }
    void CpuNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
}