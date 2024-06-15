#include <render_engine/rendergraph/Job.h>

#include <render_engine/Debugger.h>
#include <render_engine/RenderContext.h>

namespace RenderEngine::RenderGraph
{

    /*
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
    */
    uint32_t ExecutionContext::getRenderTargetIndex() const
    {
        std::shared_lock lock(_render_target_index_mutex);
        return *_render_target_index;
    }
    bool ExecutionContext::hasRenderTargetIndex() const
    {
        std::shared_lock lock(_render_target_index_mutex);
        return _render_target_index != std::nullopt;
    }
    void ExecutionContext::setRenderTargetIndex(uint32_t index)
    {
        std::unique_lock lock(_render_target_index_mutex);
        _render_target_index = index;
        on_render_target_index_set(index);
    }
    void ExecutionContext::clearRenderTargetIndex()
    {
        std::unique_lock lock(_render_target_index_mutex);
        uint32_t old_index = *_render_target_index;
        _render_target_index = std::nullopt;
        on_render_target_index_clear(old_index);
    }
    void ExecutionContext::add_events(Events events)
    {
        std::unique_lock lock(_event_mutex);
        _events.push_back(std::move(events));
    }

    void ExecutionContext::on_render_target_index_set(uint32_t index)
    {
        std::shared_lock lock(_event_mutex);
        for (auto& event : _events | std::views::filter([](const auto& events) { return events.on_render_target_index_set != nullptr; }))
        {
            event.on_render_target_index_set(index);
        }
    }

    void ExecutionContext::on_render_target_index_clear(uint32_t index)
    {
        std::shared_lock lock(_event_mutex);
        for (auto& event : _events | std::views::filter([](const auto& events) { return events.on_render_target_index_clear != nullptr; }))
        {
            event.on_render_target_index_clear(index);
        }
    }

}