#pragma once

#include <render_engine/LogicalDevice.h>
#include <render_engine/synchronization/SyncOperations.h>

#include <volk.h>

#include <mutex>
#include <vector>

namespace RenderEngine
{
    class QueueSubmitTracker
    {
    public:
        explicit QueueSubmitTracker(LogicalDevice& logical_device)
            :_logical_device(&logical_device)
        {}
        ~QueueSubmitTracker();

        QueueSubmitTracker(QueueSubmitTracker&& o) = delete;
        QueueSubmitTracker(const QueueSubmitTracker&) = delete;
        QueueSubmitTracker& operator=(QueueSubmitTracker&& o) = delete;
        QueueSubmitTracker& operator=(const QueueSubmitTracker&) = delete;

        void queueSubmit(VkSubmitInfo2&& submit_info,
                         const SyncOperations& sync_operations,
                         AbstractCommandContext& command_context);

        void wait();

        [[nodiscard]]
        uint32_t queryNumOfSuccess() const;
        [[nodiscard]]
        uint32_t getNumOfFences() const
        {
            std::lock_guard lock{ _fence_mutex };
            return static_cast<uint32_t>(_fences.size());
        }

        bool isComplete() const { return queryNumOfSuccess() == getNumOfFences(); }
        void clear();
    private:
        VkFence createFence();
        void noLockingWait();
        void noLockingClear();

        LogicalDevice* _logical_device{ nullptr };
        std::vector<VkFence> _fences;
        std::vector<VkFence> _fence_pool;
        mutable std::mutex _fence_mutex;
    };
}