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
                _operations = sync_object.getOperationsGroup(name);
                return std::move(*this);
            }
            Query&& select(std::initializer_list<std::string> name_container)&&
            {
                for (auto name : name_container)
                {
                    _operations = _operations.createUnionWith(sync_object.getOperationsGroup(name));
                }
                return std::move(*this);
            }
            Query&& extract(int32_t extract_flags)&&
            {
                _operations = _operations.extract(extract_flags);
                return std::move(*this);
            }

            Query&& join(const SyncOperations& operations)&&
            {
                _operations = _operations.createUnionWith(operations);
                return std::move(*this);
            }

            Query&& join(const Query& o)&&
            {
                _operations = _operations.createUnionWith(o._operations);
                return std::move(*this);
            }

            const SyncOperations& get()&&
            {
                return _operations;
            }
        private:
            explicit Query(const SyncObject& sync_object)
                : sync_object(sync_object)
            {}

            const SyncObject& sync_object;
            SyncOperations _operations;
        };

        SyncObject(const SyncObject&) = delete;
        SyncObject(SyncObject&&) = default;

        SyncObject& operator=(const SyncObject&) = delete;
        SyncObject& operator=(SyncObject&&) = default;

        explicit SyncObject(LogicalDevice& logical_device);

        const SyncOperations& getOperationsGroup(const std::string& name) const { return _operation_groups.at(name); }

        void addSignalOperationToGroup(const std::string& group_name,
                                       const std::string& semaphore_name,
                                       VkPipelineStageFlags2 stage_mask);
        void addSignalOperationToGroup(const std::string& group_name,
                                       const std::string& semaphore_name,
                                       VkPipelineStageFlags2 stage_mask,
                                       uint64_t value);

        void addWaitOperationToGroup(const std::string& group_name,
                                     const std::string& semaphore_name,
                                     VkPipelineStageFlags2 stage_mask);

        void addWaitOperationToGroup(const std::string& group_name,
                                     const std::string& semaphore_name,
                                     VkPipelineStageFlags2 stage_mask,
                                     uint64_t value);

        void addSemaphoreForHostOperations(const std::string& group_name,
                                           const std::string& semaphore_name);

        void signalSemaphore(const std::string& name, uint64_t value);

        void waitSemaphore(const std::string& name, uint64_t value) const;
        uint32_t waitSemaphore(const std::vector<std::string>& semaphore_names, uint64_t value) const;
        uint64_t getSemaphoreValue(const std::string& name) const;

        const SyncPrimitives& getPrimitives() const { return _primitives; }

        void createSemaphore(std::string name);

        void createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width);

        void stepTimeline(const std::string& name);

        Query query() const { return Query::from(*this); }
    private:

        SyncPrimitives _primitives;
        std::unordered_map<std::string, SyncOperations> _operation_groups;

    };
}