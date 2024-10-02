#pragma once

#include <render_engine/containers/Views.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/QueueSubmitTracker.h>
#include <render_engine/synchronization/SyncFeedbackService.h>
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

            explicit ExecutionContext(std::vector<const SyncObject*> sync_objects,
                                      SyncFeedbackService* sync_feedback_service,
                                      uint32_t id)
                : _pool_index{ PoolIndex{.render_target_index = id, .sync_object_index = id} }
                , _sync_objects(std::move(sync_objects))
                , _sync_feedback_service(sync_feedback_service)
                , _id(id)
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
            const SyncObject& getSyncObject(uint32_t sync_object_index) const
            {
                return *_sync_objects[sync_object_index];
            }

            SyncFeedbackService& getSyncFeedbackService()
            {
                return *_sync_feedback_service;
            }

            void addEvents(Events events);


            void setCurrentFrameNumber(uint64_t value)
            {
                _current_frame_number.store(value);
            }

            uint64_t getCurrentFrameNumber() const
            {
                return _current_frame_number.load();
            }

            uint32_t getId() const { return _id; }
        private:
            void onPoolIndexSet(const PoolIndex& index);
            void onPoolIndexReset(const PoolIndex& index);

            std::optional<PoolIndex> _pool_index{ std::nullopt };
            mutable std::shared_mutex _pool_index_mutex;

            std::atomic_bool _draw_call_recorded{ false };

            mutable std::shared_mutex _sync_objects_access_mutex;
            std::vector<const SyncObject*> _sync_objects;

            mutable std::shared_mutex _event_mutex;
            std::vector<Events> _events;

            SyncFeedbackService* _sync_feedback_service{ nullptr };

            std::atomic_uint64_t _current_frame_number{ 0 };

            uint32_t _id{ 0 };
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