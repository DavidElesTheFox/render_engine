#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/synchronization/SyncPrimitives.h>

#include <vector>

namespace RenderEngine
{

    class SyncOperations
    {
    public:
        enum ExtractBits : int32_t
        {
            ExtractWaitOperations = 1,
            ExtractSignalOperations = 1 << 1,
            ExtractFence = 1 << 2
        };

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

        const SyncOperations& fillInfo(VkSubmitInfo2& submit_info) const;
        bool hasAnyFence() const { return _fence != VK_NULL_HANDLE; }
        // TODO return with not a pointer
        const VkFence* getFence() const { return &_fence; }

        // TODO remove this and make a mutable object
        SyncOperations& unionWith(const SyncOperations& o);

        SyncOperations createUnionWith(const SyncOperations& o) const
        {
            SyncOperations result = *this;
            return result.unionWith(o);
        }

        void shiftTimelineSemaphoreValues(uint64_t offset);

        void clear();

        SyncOperations extract(int32_t extract_bits) const
        {
            SyncOperations result;
            if (extract_bits & ExtractWaitOperations)
            {
                result._wait_semaphore_dependency = _wait_semaphore_dependency;
            }
            if (extract_bits & ExtractSignalOperations)
            {
                result._signal_semaphore_dependency = _signal_semaphore_dependency;
            }
            if (extract_bits & ExtractFence)
            {
                result._fence = _fence;
            }
            return result;
        }

        SyncOperations restrict(const CommandContext& context) const;

    private:

        std::vector<VkSemaphoreSubmitInfo> _wait_semaphore_dependency;
        std::vector<VkSemaphoreSubmitInfo> _signal_semaphore_dependency;
        VkFence _fence{ VK_NULL_HANDLE };
    };

}