#include <render_engine/rendergraph/Node.h>

#include <render_engine/CommandContext.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/RenderContext.h>
#include <render_engine/rendergraph/GraphVisitor.h>
#include <render_engine/rendergraph/Topic.h>

#include <format>
#include <iostream>
#include <numeric>


namespace RenderEngine::RenderGraph
{
    namespace
    {
        static thread_local const std::string g_thread_name = std::format("Thread-{:}", std::this_thread::get_id());

    }
#define PROFILE_NODE() PROFILE_THREAD(g_thread_name.c_str()); PROFILE_SCOPE()
    VkCommandBuffer RenderNode::createOrGetCommandBuffer(const ExecutionContext::PoolIndex& pool_index)
    {
        std::lock_guard lock(_command_buffer_mutex);
        auto command_buffer_mapping = _command_buffers[pool_index.sync_object_index];
        auto it = command_buffer_mapping.find(pool_index.render_target_index);
        if (it == command_buffer_mapping.end())
        {
            auto command_buffer = _command_context->createCommandBuffer(pool_index.sync_object_index, pool_index.render_target_index);
            it = command_buffer_mapping.insert({ pool_index.render_target_index, command_buffer }).first;
        }
        return it->second;
    }
    void RenderNode::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void RenderNode::execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker)
    {
        PROFILE_NODE();

        /*
        * TODO: Introduce configuration for parallel-thread-count and back-buffer-count
        * Issues:
        *   To record draw calls to command-buffer 0x001 we need to be sure that 0x001 has no pending commands.
        *   It means that it is not enough to synchronize on the GPU that start to render when the image available
        *   semaphore is triggered but on the host we also need to be sure that the command buffer is ready to use.
        *
        *   We have command buffers for each back-buffer (render target image).
        *
        *   The submit tracker is thread local I.e: We only see those submition what our thread did.
        *
        *   So what can happen: (It can happen even when backbuffer count == parallel thread count but when latter is bigger then is almost for sure)
        *     thread 0 records draw calls for 0x001 (render target 0)
        *     thread 0 calss present
        *     thread 1 chooses render target 0 quickly (it's gonna be available soon)
        *     thread 1 starts recording to the command buffer but! it is not finished, because our host is super fast.
        *     findSubmitTracker doesn't work because the draw calls were submitted from thread 0.
        *
        * Ideas:
        *   - Probably the issue that we have queues and command buffers per back buffers only. We should also have one for each thread as well.
        *   - Would be cool to have a command buffer per render_target and host thread.
        */
        execution_context.findSubmitTracker(getName()).wait();

        const auto pool_index = execution_context.getPoolIndex();
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start rendering: {:s} render target: {:d} (sync index: {:d}) [thread:{}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());
        auto sync_object_holder = execution_context.getSyncObject(pool_index.sync_object_index);
        const auto& in_operations = sync_object_holder.sync_object.getOperationsGroup(getName());
        //_renderer->draw(pool_index.render_target_index);
        auto command_buffer = createOrGetCommandBuffer(pool_index);
        _renderer->draw(command_buffer, pool_index.render_target_index);

        std::vector<VkCommandBufferSubmitInfo> command_buffer_infos;

        //auto command_buffers = _renderer->getCommandBuffers(pool_index.render_target_index);
        VkCommandBufferSubmitInfo command_buffer_submit_info{
                                                         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .commandBuffer = command_buffer,
                                                       .deviceMask = 0
        };
        /*std::ranges::transform(command_buffers,
                               std::back_inserter(command_buffer_infos),
                               [](const auto& command_buffer)
                               {
                                   return VkCommandBufferSubmitInfo{
                                                       .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .commandBuffer = command_buffer,
                                                       .deviceMask = 0 };
                               });*/

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &command_buffer_submit_info;

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

        auto command_buffer_debug_str = [&] { return std::format("{:#018x}", reinterpret_cast<uintptr_t>(command_buffer)); };

        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Rendering finished: {:s} render target: {:d} command buffers: ({:s}) (sync index: {:d}) [thread: {}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     command_buffer_debug_str(),
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
        using namespace std::chrono_literals;
        PROFILE_NODE();
        // TODO currently it renders the first that finished. It is not correct and can happen that frame n is rendered later then framen n+
        const auto pool_index = execution_context.getPoolIndex();
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start presenting: {:s} render target: {:d} (sync index: {:d}) [thread: {}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());

        constexpr auto timeout = 1s;
        if (waitAndUpdateFrameNumber(execution_context.getCurrentFrameNumber(), timeout) == false)
        {
            throw std::runtime_error("Frame continuity issue, can't wait for the frame");
        }
        _frame_continuity_condition.notify_one();

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

    [[nodiscard]]
    bool PresentNode::waitAndUpdateFrameNumber(uint64_t frame_number, const std::chrono::milliseconds& timeout)
    {
        std::unique_lock lock(_frame_continuity_mutex);
        if (_current_frame_number == frame_number)
        {
            _current_frame_number++;
            return true;
        }

        const bool success = _frame_continuity_condition.wait_for(lock, timeout, [=] { return _current_frame_number == frame_number; });
        if (success)
        {
            _current_frame_number++;
        }
        return success;
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