#pragma once

#include <render_engine/QueueSubmitTracker.h>

#include <mutex>
#include <unordered_map>

namespace RenderEngine
{
    class SyncObject;
    class SyncFeedbackService
    {
    public:
        QueueSubmitTracker* get(const SyncObject* key, const std::string& name)
        {
            std::shared_lock lock(_feedbacks_mutex);
            return _feedbacks.at(key).at(name).get();
        }
        const QueueSubmitTracker* get(const SyncObject* key, const std::string& name) const
        {
            std::shared_lock lock(_feedbacks_mutex);
            return _feedbacks.at(key).at(name).get();
        }
        void registerTracker(const SyncObject* key, const std::string& name, std::unique_ptr<QueueSubmitTracker> tracker)
        {
            std::unique_lock lock(_feedbacks_mutex);
            _feedbacks[key][name] = std::move(tracker);
        }

        void clearFences()
        {
            std::unique_lock lock(_feedbacks_mutex);

            using namespace std::views;
            for (auto& mapping : _feedbacks | values)
            {
                for (auto& tracker : mapping | values)
                {
                    tracker->clear();
                }
            }
        }
    private:
        using FeedbackMapping = std::unordered_map<std::string, std::unique_ptr<QueueSubmitTracker>>;
        mutable std::shared_mutex _feedbacks_mutex;
        std::unordered_map<const SyncObject*, FeedbackMapping> _feedbacks;
    };
}