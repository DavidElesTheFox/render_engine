#pragma once

#include <render_engine/synchronization/SyncPrimitives.h>

#include <vector>

namespace RenderEngine
{

    class SyncOperations
    {
    public:
        explicit SyncOperations(VkFence fence)
            : _fence(fence)
        {}
        SyncOperations() = default;
        ~SyncOperations() = default;
        SyncOperations(const SyncOperations&) = default;
        SyncOperations(SyncOperations&&) = default;

        SyncOperations& operator=(const SyncOperations&) = default;
        SyncOperations& operator=(SyncOperations&&) = default;
        void addWaitOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask);

        void addWaitOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value);

        void addSignalOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask);

        void addSignalOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value);

        void fillInfo(VkSubmitInfo2& submit_info) const;
        bool hasAnyFence() const { return _fence != VK_NULL_HANDLE; }
        const VkFence* getFence() const { return &_fence; }

        SyncOperations& unionWith(const SyncOperations& o);

        SyncOperations createUnionWith(const SyncOperations& o) const
        {
            SyncOperations result = *this;
            return result.unionWith(o);
        }

        void shiftTimelineSemaphoreValues(uint64_t offset);

    private:

        std::vector<VkSemaphoreSubmitInfo> _wait_semaphore_dependency;
        std::vector<VkSemaphoreSubmitInfo> _signal_semaphore_dependency;
        VkFence _fence{ VK_NULL_HANDLE };
    };

}