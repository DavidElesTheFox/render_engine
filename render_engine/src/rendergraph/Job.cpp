#include <render_engine/rendergraph/Job.h>

#include <render_engine/Debugger.h>
#include <render_engine/RenderContext.h>

namespace RenderEngine::RenderGraph
{
    std::unordered_map<const SyncObject*, std::vector<std::string>> Job::ExecutionContext::SyncObjectUpdateData::clear()
    {
        std::lock_guard lock{ _timeline_semaphores_to_shift_mutex };

        auto result = _timeline_semaphores_to_shift;
        _timeline_semaphores_to_shift.clear();

        return result;
    }
    void Job::ExecutionContext::SyncObjectUpdateData::requestTimelineSemaphoreShift(const SyncObject* sync_object, const std::string& name)
    {
        std::lock_guard lock{ _timeline_semaphores_to_shift_mutex };
        _timeline_semaphores_to_shift[sync_object].push_back(name);
    }

    void Job::execute(ExecutionContext& execution_context) noexcept
    {
        try
        {
            if (_queue_tracker != nullptr)
            {
                _queue_tracker->clear();
            }
            _job(execution_context, _queue_tracker.get());
        }
        catch (const std::exception& error)
        {
            RenderContext::context().getDebugger().print("Error occurred during job execution: {:s}", error.what());
            assert(false && "Error occurred during job execution");
        }
        catch (...)
        {
            RenderContext::context().getDebugger().print("Unknown error occurred during job execution");
            assert(false && "Unknown error occurred during job execution");

        }
    }
    uint32_t Job::ExecutionContext::getRenderTargetIndex() const
    {
        std::shared_lock lock(_render_target_index_mutex);
        return *_render_target_index;
    }
    bool Job::ExecutionContext::hasRenderTargetIndex() const
    {
        std::shared_lock lock(_render_target_index_mutex);
        return _render_target_index != std::nullopt;
    }
    void Job::ExecutionContext::setRenderTargetIndex(uint32_t index)
    {
        std::unique_lock lock(_render_target_index_mutex);
        _render_target_index = index;
    }
    void Job::ExecutionContext::clearRenderTargetIndex()
    {
        std::unique_lock lock(_render_target_index_mutex);
        _render_target_index = std::nullopt;
    }


}