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

        continue here.
            /*
            * TODO: Introduce sync objects per back buffer. The only reason to wait here to be sure that the semaphores are not used by
            * the previous call.
            * With given 3 threads and 3 backbuffers all good. I.e. Each thread has its own syncObject the chance that is used is 0
            * With given 1 thread and 3 backbuffers we are basically doesn't use any backbuffers because we are always waiting for to finish the previous call
            *
            * Having sync object per back buffer will result the issue where we were:
            *  - Which sync object should be choosen when we are acquiring an image? We don't know yet which backbuffer will be used.
            *  - Submition trackers are also independent from back buffer count.
            *      Imagine the ImageAcquire wait code: We are rendered the 0, 1, 2 BackBuffers. We don't want to wait the last command, but the first
            *
            * Probably we need to link these things together into an object. Like RenderFeedback: Has a sync object and a submit tracker.
            * First try to have per thread sync objects per back buffers and these feedbacks.
            */
            execution_context.findSubmitTracker(getName()).wait();

        const auto pool_index = execution_context.getPoolIndex();
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start rendering: {:s} render target: {:d} (sync index: {:d}) [thread:{}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());
        auto& sync_object = execution_context.getSyncObject();
        const auto& in_operations = sync_object.getOperationsGroup(getName());
        //_renderer->draw(pool_index.render_target_index);
        auto command_buffer = createOrGetCommandBuffer(pool_index);
        _renderer->draw(command_buffer, pool_index.render_target_index);

        std::vector<VkCommandBufferSubmitInfo> command_buffer_infos;

        VkCommandBufferSubmitInfo command_buffer_submit_info{
                                                         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .commandBuffer = command_buffer,
                                                       .deviceMask = 0
        };

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
        auto& sync_object = execution_context.getSyncObject();
        const auto& in_operations = sync_object.getOperationsGroup(getName());
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
        auto& sync_object = execution_context.getSyncObject();
        const auto& in_operations = sync_object.getOperationsGroup(getName());
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

        constexpr auto timeout = 1s;
        if (waitAndUpdateFrameNumber(execution_context.getCurrentFrameNumber(), timeout) == false)
        {
            throw std::runtime_error("Frame continuity issue, can't wait for the frame");
        }
        _frame_continuity_condition.notify_all();

        const auto pool_index = execution_context.getPoolIndex();
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start presenting: {:s} render target: {:d} (sync index: {:d}) [thread: {}]",
                                                     getName(),
                                                     pool_index.render_target_index,
                                                     pool_index.sync_object_index,
                                                     std::this_thread::get_id());



        auto& sync_object = execution_context.getSyncObject();
        const auto& in_operations = sync_object.getOperationsGroup(getName());
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
        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Start waiting for: {:d}", frame_number);
        if (_current_frame_number == frame_number)
        {
            RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                         "Frame can present: {:d}", frame_number);
            _current_frame_number++;
            return true;
        }

        _frame_continuity_condition.wait_for(lock, timeout, [=] { return _current_frame_number == frame_number; });
        if (_current_frame_number == frame_number)
        {
            RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                         "Frame can present (2): {:d}", frame_number);
            _current_frame_number++;
            return true;
        }
        else
        {
            RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                         "Frame can't present: {:d}", frame_number);
            return false;
        }
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