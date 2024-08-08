#pragma once

#include <render_engine/containers/Views.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/QueueSubmitTracker.h>
#include <render_engine/synchronization/SyncObject.h>

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

namespace RenderEngine
{
    class QueueSubmitTracker;

    namespace RenderGraph
    {
        // TODO rename this file
        class ExecutionContext
        {
        public:
            struct PoolIndex
            {
                uint32_t render_target_index;
                uint32_t sync_object_index;
            };
            struct Events
            {
                std::function<void(const PoolIndex&)> on_pool_index_set;
                std::function<void(const PoolIndex&)> on_pool_index_clear;
            };

            explicit ExecutionContext(std::unique_ptr<SyncObject> sync_object)
                : _sync_object(std::move(sync_object))
            {}

            ExecutionContext(const ExecutionContext&) noexcept = delete;
            ExecutionContext(ExecutionContext&&) noexcept = default;

            ExecutionContext& operator=(const ExecutionContext&) noexcept = delete;
            ExecutionContext& operator=(ExecutionContext&&) noexcept = default;

            PoolIndex getPoolIndex() const;
            bool hasPoolIndex() const;
            void setPoolIndex(PoolIndex index);
            void clearPoolIndex();

            bool isDrawCallRecorded() const { return _draw_call_recorded.load(std::memory_order_relaxed); }
            void setDrawCallRecorded(bool v) { _draw_call_recorded.store(v, std::memory_order_relaxed); }
            const SyncObject& getSyncObject() const
            {
                return *_sync_object;
            }

            void stepTimelineSemaphore(const std::string& name)
            {
                PROFILE_SCOPE();
                std::unique_lock lock{ _sync_object_access_mutex };
                _sync_object->stepTimeline(name);
            }

            void addEvents(Events events);

            void addQueueSubmitTracker(const std::string& name, std::unique_ptr<QueueSubmitTracker> submit_tracker)
            {
                std::unique_lock lock(_submit_trackers_mutex);
                _submit_trackers.insert({ name, std::move(submit_tracker) });
            }

            const QueueSubmitTracker& findSubmitTracker(const std::string& name) const
            {
                PROFILE_SCOPE();
                std::shared_lock lock(_submit_trackers_mutex);
                return *_submit_trackers.at(name);
            }

            void clearSubmitTrackersPool()
            {
                PROFILE_SCOPE();
                std::unique_lock lock(_submit_trackers_mutex);
                for (auto& submit_tracker : _submit_trackers)
                {
                    submit_tracker.second->clear();
                }
            }

            void setCurrentFrameNumber(uint64_t value)
            {
                _current_frame_number.store(value, std::memory_order_relaxed);
            }

            uint64_t getCurrentFrameNumber() const
            {
                return _current_frame_number.load(std::memory_order_relaxed);
            }
        private:
            void onPoolIndexSet(const PoolIndex& index);
            void onPoolIndexReset(const PoolIndex& index);

            std::optional<PoolIndex> _pool_index{ std::nullopt };
            mutable std::shared_mutex _pool_index_mutex;

            std::atomic_bool _draw_call_recorded{ false };

            mutable std::shared_mutex _sync_object_access_mutex;
            std::unique_ptr<SyncObject> _sync_object;

            mutable std::shared_mutex _event_mutex;
            std::vector<Events> _events;

            mutable std::shared_mutex _submit_trackers_mutex;
            std::unordered_map<std::string, std::unique_ptr<QueueSubmitTracker>> _submit_trackers;

            std::atomic_uint64_t _current_frame_number{ 0 };
        };
        /*
        class Job
        {
        public:

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
            void assignExecutionContext(ExecutionContext& execution_context) { onAssignExecutionContext(execution_context); }
        private:
            virtual void onAssignExecutionContext(ExecutionContext&) {};
            std::string _name;
            std::function<void(ExecutionContext&, QueueSubmitTracker*)> _job;
            std::unique_ptr<QueueSubmitTracker> _queue_tracker;
        };
        */
    }
}