#pragma once

#include <render_engine/synchronization/SyncOperations.h>
#include <render_engine/synchronization/SyncPrimitives.h>

namespace RenderEngine
{
    /**
    * Synchronization group namespace for publicly available groups.
    *
    * Usually an object want to use semaphores in two direction.
    *   - One usage is a internal/private usage.
    *     It is about how the object would like to use internally the semaphore. For example signaling it or waiting for it.
    *   - The other is an external/interface usage. It is about how it can be used from outside. For example the next operation needs
    *     to wait for the semaphore.
    */
    namespace SyncGroups
    {

        constexpr auto kInternal = "InternalGroup";
        constexpr auto kExternal = "ExternalGroup";
    }

    class SyncObject
    {
    public:
        static SyncObject CreateEmpty(VkDevice logical_device)
        {
            return SyncObject(logical_device, false);
        }
        static SyncObject CreateWithFence(VkDevice logical_device, VkFenceCreateFlags create_flags)
        {
            return SyncObject(logical_device, true, create_flags);
        }
        const SyncOperations& getOperationsGroup(const std::string& name) const { return _operation_groups.at(name); }

        void addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask);
        void addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value);

        void addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask);

        void addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value);

        void signalSemaphore(const std::string& name, uint64_t value);

        void waitSemaphore(const std::string& name, uint64_t value);

        const SyncPrimitives& getPrimitives() const { return _primitives; }

        void createSemaphore(std::string name);

        void createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width);

        void stepTimeline(const std::string& name);

    private:
        SyncObject(VkDevice logical_device, bool create_with_fence, VkFenceCreateFlags create_flags = 0);

        SyncPrimitives _primitives;
        std::unordered_map<std::string, SyncOperations> _operation_groups;
    };
}