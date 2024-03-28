#include <render_engine/rendergraph/Job.h>

#include <render_engine/Debugger.h>
#include <render_engine/RenderContext.h>

namespace RenderEngine::RenderGraph
{
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
    void Job::ExecutionContext::reset()
    {
        if (isDrawCallRecorded())
        {
            std::unique_lock lock(_render_target_index_mutex);
            _render_target_index = std::nullopt;
        }
    }
}