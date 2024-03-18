#pragma once

#include <render_engine/LogicalDevice.h>
#include <render_engine/synchronization/SyncOperations.h>

#include <volk.h>

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

        QueueSubmitTracker(QueueSubmitTracker&& o) noexcept
            : _logical_device(o._logical_device)
            , _fences(std::move(o._fences))
        {
            o._logical_device = nullptr;
        }
        QueueSubmitTracker(const QueueSubmitTracker&) = delete;
        QueueSubmitTracker& operator=(QueueSubmitTracker&& o) noexcept
        {
            using std::swap;
            swap(_logical_device, o._logical_device);
            swap(_fences, o._fences);
            return *this;
        }
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
            return static_cast<uint32_t>(_fences.size());
        }

        bool isComplete() const { return queryNumOfSuccess() == getNumOfFences(); }
        void clear();
    private:
        LogicalDevice* _logical_device{ nullptr };
        // TODO implement object pool for fences
        std::vector<VkFence> _fences;
    };
}