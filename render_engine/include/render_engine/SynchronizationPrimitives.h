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
            , _logical_device(o._logical_device)
            , _fence(o._fence)
        {
            o._logical_device = VK_NULL_HANDLE;
            o._fence = VK_NULL_HANDLE;
            o._semaphore_map.clear();
        }
        SynchronizationPrimitives(const SynchronizationPrimitives& o) = delete;
        SynchronizationPrimitives& operator=(SynchronizationPrimitives&& o)
        {
            std::swap(o._semaphore_map, _semaphore_map);
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

        const VkSemaphore& getSemaphore(const std::string& name) const
        {
            return _semaphore_map.at(name);
        }

        VkFence getFence() const { return _fence; }
    private:
        SynchronizationPrimitives(VkDevice logical_device) :
            _logical_device(logical_device)
        {}

        std::unordered_map<std::string, VkSemaphore> _semaphore_map;
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

        void addSignalOperation(SynchronizationPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            VkSemaphoreSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
            submit_info.stageMask = stage_mask;
            _signal_semaphore_dependency.emplace_back(std::move(submit_info));
        }

        void fillInfo(VkSubmitInfo2& submit_info)
        {
            remapTimelineData();
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
            for (auto& [semaphore_index, timeline_semaphore] : o._wait_timeline_semaphore_map)
            {
                bool inserted = _wait_timeline_semaphore_map.emplace(semaphore_index + wait_semaphore_offset, timeline_semaphore).second;
                assert(inserted && "Probably something got overriden in the map");
            }
            for (auto& [semaphore_index, timeline_semaphore] : o._signal_timeline_semaphore_map)
            {
                bool inserted = _signal_timeline_semaphore_map.emplace(semaphore_index + signal_semaphore_offset, timeline_semaphore).second;
                assert(inserted && "Probably something got overriden in the map");
            }
            return *this;
        }

        SyncOperations createUnionWith(const SyncOperations& o) const
        {
            SyncOperations result = *this;
            return result.unionWith(o);
        }

    private:
        // due to moving around the object the pNext might be broken, needs remap.
        void remapTimelineData()
        {
            for (uint32_t semaphore_idx = 0; semaphore_idx < _wait_semaphore_dependency.size(); ++semaphore_idx)
            {
                if (auto it = _wait_timeline_semaphore_map.find(semaphore_idx); it != _wait_timeline_semaphore_map.end())
                {
                    _wait_semaphore_dependency[semaphore_idx].pNext = &it->second;
                }
            }
            for (uint32_t semaphore_idx = 0; semaphore_idx < _signal_semaphore_dependency.size(); ++semaphore_idx)
            {
                if (auto it = _signal_timeline_semaphore_map.find(semaphore_idx); it != _signal_timeline_semaphore_map.end())
                {
                    _signal_semaphore_dependency[semaphore_idx].pNext = &it->second;
                }
            }
        }

        std::vector<VkSemaphoreSubmitInfo> _wait_semaphore_dependency;
        std::unordered_map<uint32_t, VkTimelineSemaphoreSubmitInfo> _wait_timeline_semaphore_map;
        std::vector<VkSemaphoreSubmitInfo> _signal_semaphore_dependency;
        std::unordered_map<uint32_t, VkTimelineSemaphoreSubmitInfo> _signal_timeline_semaphore_map;
        VkFence _fence{ VK_NULL_HANDLE };
    };

    class SynchronizationObject
    {
    public:
        static constexpr auto kDefaultOperationGroup = "Default";

        static SynchronizationObject CreateEmpty(VkDevice logical_device)
        {
            return SynchronizationObject(logical_device, false);
        }
        static SynchronizationObject CreateWithFence(VkDevice logical_device, VkFenceCreateFlags create_flags)
        {
            return SynchronizationObject(logical_device, true, create_flags);
        }
        const SyncOperations& getDefaultOperations() const { return _operation_groups.at(kDefaultOperationGroup); }
        const SyncOperations& getOperationsGroup(const std::string& name) const { return _operation_groups.at(name); }

        void addSignalOperation(const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            _operation_groups.at(kDefaultOperationGroup).addSignalOperation(_primitives, semaphore_name, stage_mask);
        }

        void addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            auto it = _operation_groups.find(group_name);
            if (it == _operation_groups.end())
            {
                it = _operation_groups.insert({ group_name, SyncOperations{_primitives.getFence()} }).first;
            }
            it->second.addSignalOperation(_primitives, semaphore_name, stage_mask);
        }

        void addWaitOperation(SynchronizationObject& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            _operation_groups.at(kDefaultOperationGroup).addWaitOperation(sync_object._primitives, semaphore_name, stage_mask);
        }

        void addWaitOperationToGroup(const std::string& group_name, SynchronizationObject& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
        {
            auto it = _operation_groups.find(group_name);
            if (it == _operation_groups.end())
            {
                it = _operation_groups.insert({ group_name, SyncOperations{_primitives.getFence()} }).first;
            }
            it->second.addWaitOperation(_primitives, semaphore_name, stage_mask);
        }
        const SynchronizationPrimitives& getPrimitives() const { return _primitives; }

        void createSemaphore(std::string name)
        {
            _primitives.createSemaphore(std::move(name));
        }
    private:

        SynchronizationObject(VkDevice logical_device, bool create_with_fence, VkFenceCreateFlags create_flags = 0)
            : _primitives(create_with_fence ? SynchronizationPrimitives::CreateWithFence(logical_device, create_flags)
                          : SynchronizationPrimitives::CreateEmpty(logical_device))
            , _operation_groups{ { std::string(kDefaultOperationGroup), SyncOperations{_primitives.getFence()} } }
        {}
        SynchronizationPrimitives _primitives;
        std::unordered_map<std::string, SyncOperations> _operation_groups;
    };
    inline void connectVia(const std::string& semaphore_name,
                           SynchronizationObject& signaling_object, VkPipelineStageFlags2 signal_stage,
                           SynchronizationObject& wait_object, VkPipelineStageFlagBits2 wait_stage)
    {
        signaling_object.addSignalOperation(semaphore_name, signal_stage);
        wait_object.addWaitOperation(signaling_object, semaphore_name, wait_stage);
    }
}