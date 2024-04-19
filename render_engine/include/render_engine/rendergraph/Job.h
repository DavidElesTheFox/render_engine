#pragma once

#include <render_engine/synchronization/SyncObject.h>

#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>

namespace RenderEngine
{
    class QueueSubmitTracker;

    namespace RenderGraph
    {

        class Job
        {
        public:
            class ExecutionContext
            {
            public:
                explicit ExecutionContext(std::vector<std::unique_ptr<const SyncObject>>);

                ExecutionContext(const ExecutionContext&) noexcept = delete;
                ExecutionContext(ExecutionContext&&) noexcept = default;

                ExecutionContext& operator=(const ExecutionContext&) noexcept = delete;
                ExecutionContext& operator=(ExecutionContext&&) noexcept = default;

                uint32_t getRenderTargetIndex() const;
                bool hasRenderTargetIndex() const;
                void setRenderTargetIndex(uint32_t index);

                void reset();
                bool isDrawCallRecorded() const { return _draw_call_recorded.load(std::memory_order_relaxed); }
                void setDrawCallRecorded(bool v) { _draw_call_recorded.store(v, std::memory_order_relaxed); }
                const SyncObject& getSyncObject(uint32_t render_target_index) const;
                std::vector<const SyncObject*> getAllSyncObject() const;
            private:
                std::optional<uint32_t> _render_target_index{ std::nullopt };
                std::atomic_bool _draw_call_recorded{ false };
                mutable std::shared_mutex _render_target_index_mutex;
                std::vector<std::unique_ptr<const SyncObject>> _sync_objects;
            };


            Job(std::string name,
                std::function<void(ExecutionContext&, QueueSubmitTracker*)> job,
                std::unique_ptr<QueueSubmitTracker> queue_tracker = nullptr)
                : _name(std::move(name))
                , _job(std::move(job))
                , _queue_tracker(std::move(queue_tracker))
            {}

            Job(Job&&) = delete;
            Job(const Job&) = delete;

            Job& operator=(Job&&) = delete;
            Job& operator=(const Job&) = delete;

            const std::string& getName() const { return _name; }
            void execute(ExecutionContext&) noexcept;
            const QueueSubmitTracker* getQueueTracker() const { return _queue_tracker.get(); }
        private:
            std::string _name;
            std::function<void(ExecutionContext&, QueueSubmitTracker*)> _job;
            std::unique_ptr<QueueSubmitTracker> _queue_tracker;
        };
    }
}