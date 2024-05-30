#pragma once

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

        class Job
        {
        public:
            class ExecutionContext
            {
            public:
                class SyncObjectUpdateData
                {
                public:
                    void requestTimelineSemaphoreShift(const SyncObject* sync_object, const std::string& name);
                    std::unordered_map<const SyncObject*, std::vector<std::string>> clear();
                private:
                    std::unordered_map<const SyncObject*, std::vector<std::string>> _timeline_semaphores_to_shift;
                    std::mutex _timeline_semaphores_to_shift_mutex;
                };
                explicit ExecutionContext(std::vector<const SyncObject*> sync_objects)
                    : _sync_objects(std::move(sync_objects))
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
                const SyncObject& getSyncObject(uint32_t render_target_index) const
                {
                    return *_sync_objects[render_target_index];
                }
                const std::vector<const SyncObject*>& getAllSyncObject() const
                {
                    return _sync_objects;
                }

                SyncObjectUpdateData& getSyncObjectUpdateData() { return _sync_object_update_data; }
            private:
                std::optional<uint32_t> _render_target_index{ std::nullopt };
                std::atomic_bool _draw_call_recorded{ false };
                mutable std::shared_mutex _render_target_index_mutex;
                std::vector<const SyncObject*> _sync_objects;

                SyncObjectUpdateData _sync_object_update_data;
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