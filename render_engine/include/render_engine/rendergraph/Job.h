#pragma once

#include <render_engine/containers/Views.h>
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
        class ExecutionContext
        {
        public:
            struct Events
            {
                std::function<void(uint32_t)> on_render_target_index_set;
                std::function<void(uint32_t)> on_render_target_index_clear;
            };

            explicit ExecutionContext(std::vector<SyncObject*> sync_objects)
                : _sync_objects(std::move(sync_objects))
                , _const_sync_objects(_sync_objects | views::to<std::vector<const SyncObject*>>())
            {}

            ExecutionContext(const ExecutionContext&) noexcept = delete;
            ExecutionContext(ExecutionContext&&) noexcept = default;

            ExecutionContext& operator=(const ExecutionContext&) noexcept = delete;
            ExecutionContext& operator=(ExecutionContext&&) noexcept = default;

            uint32_t getRenderTargetIndex() const;
            bool hasRenderTargetIndex() const;
            void setRenderTargetIndex(uint32_t index);
            void clearRenderTargetIndex();

            bool isDrawCallRecorded() const { return _draw_call_recorded.load(std::memory_order_relaxed); }
            void setDrawCallRecorded(bool v) { _draw_call_recorded.store(v, std::memory_order_relaxed); }
            auto getSyncObject(uint32_t render_target_index) const
            {
                struct Result
                {
                    Result(const SyncObject& sync_object, std::shared_lock<std::shared_mutex>&& lock)
                        : lock(std::move(lock))
                        , sync_object(sync_object)
                    {}
                    std::shared_lock<std::shared_mutex> lock;
                    const SyncObject& sync_object;
                };
                auto lock = std::shared_lock{ _sync_object_access_mutex };
                return Result{ *_sync_objects[render_target_index], std::shared_lock{ _sync_object_access_mutex } };
            }
            auto getAllSyncObject() const
            {
                struct Result
                {
                    Result(const std::vector<const SyncObject*>& sync_objects, std::shared_lock<std::shared_mutex>&& lock)
                        : sync_objects(sync_objects)
                        , lock(std::move(lock))
                    {}
                    const std::vector<const SyncObject*>& sync_objects;
                    std::shared_lock<std::shared_mutex> lock;
                };
                return Result{ _const_sync_objects, std::shared_lock{_sync_object_access_mutex} };
            }

            void stepTimelineSemaphore(uint32_t render_target_index, const std::string& name)
            {
                std::unique_lock lock{ _sync_object_access_mutex };
                _sync_objects[render_target_index]->stepTimeline(name);
            }

            void add_events(Events events);
        private:
            std::optional<uint32_t> _render_target_index{ std::nullopt };
            std::atomic_bool _draw_call_recorded{ false };
            mutable std::shared_mutex _render_target_index_mutex;

            mutable std::shared_mutex _sync_object_access_mutex;
            std::vector<SyncObject*> _sync_objects;
            std::vector<const SyncObject*> _const_sync_objects;

            mutable std::shared_mutex _event_mutex;
            std::vector<Events> _events;


            void on_render_target_index_set(uint32_t index);
            void on_render_target_index_clear(uint32_t index);
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