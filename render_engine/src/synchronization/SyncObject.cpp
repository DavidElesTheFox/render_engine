#include <render_engine/synchronization/SyncObject.h>

#include <ranges>
#include <stdexcept>

namespace RenderEngine
{
    void SyncObject::addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
    {
        auto it = _operation_groups.find(group_name);
        if (it == _operation_groups.end())
        {
            it = _operation_groups.insert({ group_name, SyncOperations{ _primitives.getFence() } }).first;
        }
        it->second.addSignalOperation(_primitives, semaphore_name, stage_mask);
    }

    void SyncObject::addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        auto it = _operation_groups.find(group_name);
        if (it == _operation_groups.end())
        {
            it = _operation_groups.insert({ group_name, SyncOperations{ _primitives.getFence() } }).first;
        }
        it->second.addSignalOperation(_primitives, semaphore_name, stage_mask, value);
    }
    void SyncObject::addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
    {
        auto it = _operation_groups.find(group_name);
        if (it == _operation_groups.end())
        {
            it = _operation_groups.insert({ group_name, SyncOperations{ _primitives.getFence() } }).first;
        }
        it->second.addWaitOperation(_primitives, semaphore_name, stage_mask);
    }
    void SyncObject::addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        auto it = _operation_groups.find(group_name);
        if (it == _operation_groups.end())
        {
            it = _operation_groups.insert({ group_name, SyncOperations{ _primitives.getFence() } }).first;
        }
        it->second.addWaitOperation(_primitives, semaphore_name, stage_mask, value);
    }
    void SyncObject::signalSemaphore(const std::string& name, uint64_t value)
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
    void SyncObject::waitSemaphore(const std::string& name, uint64_t value)
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
    void SyncObject::createSemaphore(std::string name)
    {
        _primitives.createSemaphore(std::move(name));
    }
    void SyncObject::createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width)
    {
        _primitives.createTimelineSemaphore(std::move(name), initial_value, timeline_width);
    }
    void SyncObject::stepTimeline(const std::string& name)
    {
        const uint64_t offset = _primitives.stepTimeline(name);
        for (auto& sync_operation : _operation_groups | std::views::values)
        {
            sync_operation.shiftTimelineSemaphoreValues(offset);
        }
    }

    SyncObject::SyncObject(VkDevice logical_device, bool create_with_fence, VkFenceCreateFlags create_flags)
        : _primitives(create_with_fence ? SyncPrimitives::CreateWithFence(logical_device, create_flags)
                      : SyncPrimitives::CreateEmpty(logical_device))
        , _operation_groups{ { std::string(SyncGroups::kInternal), SyncOperations{_primitives.getFence()} },
        { std::string(SyncGroups::kExternal), SyncOperations{_primitives.getFence()} } }
    {}
}