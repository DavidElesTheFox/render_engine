#include <render_engine/rendergraph/Job.h>

#include <render_engine/Debugger.h>
#include <render_engine/RenderContext.h>

namespace RenderEngine::RenderGraph
{
    ExecutionContext::PoolIndex ExecutionContext::getPoolIndex() const
    {
        std::shared_lock lock(_pool_index_mutex);
        return *_pool_index;
    }
    bool ExecutionContext::hasPoolIndex() const
    {
        std::shared_lock lock(_pool_index_mutex);
        return _pool_index != std::nullopt;
    }
    void ExecutionContext::setPoolIndex(ExecutionContext::PoolIndex index)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_pool_index_mutex);
        _pool_index = std::move(index);
        onPoolIndexSet(*_pool_index);
    }
    void ExecutionContext::clearPoolIndex()
    {
        PROFILE_SCOPE();
        PoolIndex old_index;
        {
            std::unique_lock lock(_pool_index_mutex);
            old_index = *_pool_index;
            _pool_index = std::nullopt;
        }
        onPoolIndexReset(old_index);
    }
    void ExecutionContext::addEvents(Events events)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_event_mutex);
        _events.push_back(std::move(events));
    }

    void ExecutionContext::onPoolIndexSet(const PoolIndex& index)
    {
        std::shared_lock lock(_event_mutex);
        for (auto& event : _events | std::views::filter([](const auto& events) { return events.on_pool_index_set != nullptr; }))
        {
            event.on_pool_index_set(index);
        }
    }

    void ExecutionContext::onPoolIndexReset(const PoolIndex& index)
    {
        PROFILE_SCOPE();
        std::shared_lock lock(_event_mutex);
        for (auto& event : _events | std::views::filter([](const auto& events) { return events.on_pool_index_clear != nullptr; }))
        {
            event.on_pool_index_clear(index);
        }
    }

}