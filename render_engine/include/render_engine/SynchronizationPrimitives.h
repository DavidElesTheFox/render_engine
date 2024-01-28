#pragma once

#include <volk.h>

#include <cassert>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace RenderEngine
{
    class SynchronizationPrimitives
    {
    public:
        static SynchronizationPrimitives CreateWithFence(VkDevice logical_device, VkFenceCreateFlags flags)
        {
            SynchronizationPrimitives synchronization_primitives{ logical_device };
            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = flags;
            VkFence fence;
            if (vkCreateFence(logical_device, &fence_info, nullptr, &fence) != VK_SUCCESS)
            {
                throw std::runtime_error("Cannot create fence for upload");
            }
            synchronization_primitives._fence = fence;
            synchronization_primitives._logical_device = logical_device;
            return synchronization_primitives;
        }
        static SynchronizationPrimitives CreateEmpty(VkDevice logical_device)
        {
            return { logical_device };
        }

        ~SynchronizationPrimitives()
        {
            if (_fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(_logical_device, _fence, nullptr);
            }
            for (VkSemaphore semaphore : _semaphore_map | std::views::values)
            {
                vkDestroySemaphore(_logical_device, semaphore, nullptr);
            }
        }
        SynchronizationPrimitives(SynchronizationPrimitives&& o)
            : _semaphore_map(std::move(o._semaphore_map))
            , _timeline_data(std::move(o._timeline_data))
            , _logical_device(o._logical_device)
            , _fence(o._fence)
        {
            o._logical_device = VK_NULL_HANDLE;
            o._fence = VK_NULL_HANDLE;
            o._semaphore_map.clear();
            o._timeline_data.clear();
        }
        SynchronizationPrimitives(const SynchronizationPrimitives& o) = delete;
        SynchronizationPrimitives& operator=(SynchronizationPrimitives&& o)
        {
            std::swap(o._semaphore_map, _semaphore_map);
            std::swap(o._timeline_data, _timeline_data);
            std::swap(o._logical_device, _logical_device);
            std::swap(o._fence, _fence);
            return *this;
        }
        SynchronizationPrimitives& operator=(const SynchronizationPrimitives& o) = delete;

        void createSemaphore(std::string name)
        {
            VkSemaphoreCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkSemaphore semaphore;
            if (vkCreateSemaphore(_logical_device, &create_info, nullptr, &semaphore) != VK_SUCCESS)
            {
                throw std::runtime_error("Cannot create semaphore");
            }
            _semaphore_map.emplace(std::move(name), semaphore);
        }

        void createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width)
        {
            VkSemaphoreTypeCreateInfo type_info{};
            type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            type_info.initialValue = initial_value;

            VkSemaphoreCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            create_info.pNext = &type_info;
            VkSemaphore semaphore;
            if (vkCreateSemaphore(_logical_device, &create_info, nullptr, &semaphore) != VK_SUCCESS)
            {
                throw std::runtime_error("Cannot create semaphore");
            }
            _semaphore_map.emplace(name, semaphore);
            _timeline_data.emplace(std::move(name),
                                   TimelineSemaphoreData{ .timeline_width = timeline_width, .initial_value = initial_value });
        }

        const VkSemaphore& getSemaphore(const std::string& name) const
        {
            return _semaphore_map.at(name);
        }

        VkFence getFence() const { return _fence; }

        VkDevice getLogicalDevice() const { return _logical_device; }

        uint64_t getTimelineOffset(const std::string& name)
        {
            return _timeline_data.at(name).timeline_offset;
        }

        uint64_t stepTimeline(const std::string& name)
        {
            auto& timeline_data = _timeline_data.at(name);
            // TODO recreate Timeline semaphore when overflow happened. initial value is stored so probably it is a small fix.
            assert(timeline_data.timeline_offset < timeline_data.timeline_offset + timeline_data.timeline_width
                   && "Overflow happened. Semaphore needs to be recreated to be able to continue");

            timeline_data.timeline_offset += timeline_data.timeline_width;
            return timeline_data.timeline_width;
        }
    private:
        struct TimelineSemaphoreData
        {
            uint64_t timeline_width{ 0 };
            uint64_t timeline_offset{ 0 };
            uint64_t initial_value{ 0 };
        };
        SynchronizationPrimitives(VkDevice logical_device) :
            _logical_device(logical_device)
        {}

        std::unordered_map<std::string, VkSemaphore> _semaphore_map;
        std::unordered_map<std::string, TimelineSemaphoreData> _timeline_data;
        VkFence _fence{ VK_NULL_HANDLE };

        VkDevice _logical_device{ VK_NULL_HANDLE };
    };

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
        void addWaitOperation(SynchronizationPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            VkSemaphoreSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
            submit_info.stageMask = stage_mask;
            _wait_semaphore_dependency.emplace_back(std::move(submit_info));
        }

        void addWaitOperation(SynchronizationPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
        {
            VkSemaphoreSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
            submit_info.stageMask = stage_mask;
            submit_info.value = sync_object.getTimelineOffset(semaphore_name) + value;
            _wait_semaphore_dependency.emplace_back(std::move(submit_info));
        }

        void addSignalOperation(SynchronizationPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            VkSemaphoreSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
            submit_info.stageMask = stage_mask;
            _signal_semaphore_dependency.emplace_back(std::move(submit_info));
        }

        void addSignalOperation(SynchronizationPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
        {
            VkSemaphoreSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
            submit_info.stageMask = stage_mask;
            submit_info.value = sync_object.getTimelineOffset(semaphore_name) + value;
            _signal_semaphore_dependency.emplace_back(std::move(submit_info));
        }

        void fillInfo(VkSubmitInfo2& submit_info) const
        {
            submit_info.waitSemaphoreInfoCount = _wait_semaphore_dependency.size();
            submit_info.pWaitSemaphoreInfos = _wait_semaphore_dependency.data();

            submit_info.signalSemaphoreInfoCount = _signal_semaphore_dependency.size();
            submit_info.pSignalSemaphoreInfos = _signal_semaphore_dependency.data();
        }
        bool hasAnyFence() const { return _fence != VK_NULL_HANDLE; }
        const VkFence* getFence() const { return &_fence; }

        SyncOperations& unionWith(const SyncOperations& o)
        {
            uint32_t wait_semaphore_offset = _wait_semaphore_dependency.size();
            uint32_t signal_semaphore_offset = _signal_semaphore_dependency.size();

            std::ranges::copy(o._wait_semaphore_dependency, std::back_inserter(_wait_semaphore_dependency));
            std::ranges::copy(o._signal_semaphore_dependency, std::back_inserter(_signal_semaphore_dependency));
            assert((_fence == VK_NULL_HANDLE || o._fence == VK_NULL_HANDLE) && "fence conflict");
            if (_fence == VK_NULL_HANDLE)
            {
                _fence = o._fence;
            }
            return *this;
        }

        SyncOperations createUnionWith(const SyncOperations& o) const
        {
            SyncOperations result = *this;
            return result.unionWith(o);
        }

        void shiftTimelineSemaphoreValues(uint64_t offset)
        {
            for (auto& submit_info : _signal_semaphore_dependency)
            {
                // Shifting all values is not an issue, because this field is used only for timeline semaphores
                submit_info.value += offset;
            }
            for (auto& submit_info : _wait_semaphore_dependency)
            {
                // Shifting all values is not an issue, because this field is used only for timeline semaphores
                submit_info.value += offset;
            }
        }

    private:

        std::vector<VkSemaphoreSubmitInfo> _wait_semaphore_dependency;
        std::vector<VkSemaphoreSubmitInfo> _signal_semaphore_dependency;
        VkFence _fence{ VK_NULL_HANDLE };
    };

    namespace SyncGroups
    {
        constexpr auto kInner = "InnerGroup";
        constexpr auto kOuter = "OuterGroup";
    }

    class SynchronizationObject
    {
    public:


        static SynchronizationObject CreateEmpty(VkDevice logical_device)
        {
            return SynchronizationObject(logical_device, false);
        }
        static SynchronizationObject CreateWithFence(VkDevice logical_device, VkFenceCreateFlags create_flags)
        {
            return SynchronizationObject(logical_device, true, create_flags);
        }
        const SyncOperations& getOperationsGroup(const std::string& name) const { return _operation_groups.at(name); }

        void addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            auto it = _operation_groups.find(group_name);
            if (it == _operation_groups.end())
            {
                it = _operation_groups.insert({ group_name, SyncOperations{_primitives.getFence()} }).first;
            }
            it->second.addSignalOperation(_primitives, semaphore_name, stage_mask);
        }

        void addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            auto it = _operation_groups.find(group_name);
            if (it == _operation_groups.end())
            {
                it = _operation_groups.insert({ group_name, SyncOperations{_primitives.getFence()} }).first;
            }
            it->second.addWaitOperation(_primitives, semaphore_name, stage_mask);
        }

        void addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
        {
            auto it = _operation_groups.find(group_name);
            if (it == _operation_groups.end())
            {
                it = _operation_groups.insert({ group_name, SyncOperations{_primitives.getFence()} }).first;
            }
            it->second.addWaitOperation(_primitives, semaphore_name, stage_mask, value);
        }

        void signalSemaphore(const std::string& name, uint64_t value)
        {
            auto semaphore = _primitives.getSemaphore(name);

            VkSemaphoreSignalInfo signal_info{};
            signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signal_info.semaphore = semaphore;
            signal_info.value = _primitives.getTimelineOffset(name) + value;
            if (vkSignalSemaphore(_primitives.getLogicalDevice(), &signal_info) != VK_SUCCESS)
            {
                throw std::runtime_error("Couldn't signal semaphore: " + name);
            }
        }

        void waitSemaphore(const std::string& name, uint64_t value)
        {
            auto semaphore = _primitives.getSemaphore(name);

            const uint64_t value_to_wait = _primitives.getTimelineOffset(name) + value;

            VkSemaphoreWaitInfo wait_info{};
            wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            wait_info.pSemaphores = &semaphore;
            wait_info.semaphoreCount = 1;
            wait_info.pValues = &value_to_wait;
            if (vkWaitSemaphores(_primitives.getLogicalDevice(), &wait_info, UINT64_MAX) != VK_SUCCESS)
            {
                throw std::runtime_error("Couldn't signal semaphore: " + name);
            }
        }

        const SynchronizationPrimitives& getPrimitives() const { return _primitives; }

        void createSemaphore(std::string name)
        {
            _primitives.createSemaphore(std::move(name));
        }

        void createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width)
        {
            _primitives.createTimelineSemaphore(std::move(name), initial_value, timeline_width);
        }

        void stepTimeline(const std::string& name)
        {
            const uint64_t offset = _primitives.stepTimeline(name);
            for (auto& sync_operation : _operation_groups | std::views::values)
            {
                sync_operation.shiftTimelineSemaphoreValues(offset);
            }
        }

    private:

        SynchronizationObject(VkDevice logical_device, bool create_with_fence, VkFenceCreateFlags create_flags = 0)
            : _primitives(create_with_fence ? SynchronizationPrimitives::CreateWithFence(logical_device, create_flags)
                          : SynchronizationPrimitives::CreateEmpty(logical_device))
            , _operation_groups{ { std::string(SyncGroups::kInner), SyncOperations{_primitives.getFence()} },
            { std::string(SyncGroups::kOuter), SyncOperations{_primitives.getFence()} } }
        {}
        SynchronizationPrimitives _primitives;
        std::unordered_map<std::string, SyncOperations> _operation_groups;
    };
}