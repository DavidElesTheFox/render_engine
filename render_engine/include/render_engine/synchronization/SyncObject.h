#pragma once

#include <render_engine/LogicalDevice.h>
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
        class Query
        {
        public:
            static Query from(const SyncObject& sync_object)
            {
                return Query{ sync_object };
            }

            Query&& select(const std::string& name)&&
            {
                _operations = _sync_object.getOperationsGroup(name);
                return std::move(*this);
            }
            Query&& select(std::initializer_list<std::string> name_container)&&
            {
                // Fences are needed only once to avoid 'fence-conflict' See more in the unionWith function
                bool need_fence = true;
                for (auto name : name_container)
                {
                    if (need_fence == false)
                    {
                        constexpr int32_t everything_except_fence_bit = ~SyncOperations::ExtractFence;
                        _operations.unionWith(_sync_object.getOperationsGroup(name).extract(everything_except_fence_bit));
                    }
                    else
                    {
                        need_fence = false;
                        _operations.unionWith(_sync_object.getOperationsGroup(name));
                    }
                }
                return std::move(*this);
            }
            Query&& extract(int32_t extract_flags)&&
            {
                _operations.extract(extract_flags);
                return std::move(*this);
            }

            Query&& join(const SyncOperations& operations)&&
            {
                _operations.unionWith(operations);
                return std::move(*this);
            }

            Query&& join(const Query& o)&&
            {
                _operations.unionWith(o._operations);
                return std::move(*this);
            }

            const SyncOperations& get()&&
            {
                return _operations;
            }
        private:
            Query(const SyncObject& sync_object)
                : _sync_object(sync_object)
            {}

            const SyncObject& _sync_object;
            SyncOperations _operations;
        };

        SyncObject(const SyncObject&) = delete;
        SyncObject(SyncObject&&) = default;

        SyncObject& operator=(const SyncObject&) = delete;
        SyncObject& operator=(SyncObject&&) = default;

        static SyncObject CreateEmpty(LogicalDevice& logical_device)
        {
            return SyncObject(logical_device, false);
        }
        static SyncObject CreateWithFence(LogicalDevice& logical_device, VkFenceCreateFlags create_flags)
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
        uint64_t getSemaphoreValue(const std::string& name);

        void waitFence();
        void resetFence();

        const SyncPrimitives& getPrimitives() const { return _primitives; }

        void createSemaphore(std::string name);

        void createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width);

        void stepTimeline(const std::string& name);

        Query query() const { return Query::from(*this); }

    private:
        SyncObject(LogicalDevice& logical_device, bool create_with_fence, VkFenceCreateFlags create_flags = 0);

        SyncPrimitives _primitives;
        std::unordered_map<std::string, SyncOperations> _operation_groups;
    };
}