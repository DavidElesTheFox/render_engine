#include <render_engine/synchronization/SyncObject.h>

#include <ranges>
#include <stdexcept>

namespace RenderEngine
{
    void SyncObject::addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
    {
        _operation_groups[group_name].addSignalOperation(_primitives, semaphore_name, stage_mask);
    }

    void SyncObject::addSignalOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        _operation_groups[group_name].addSignalOperation(_primitives, semaphore_name, stage_mask, value);
    }
    void SyncObject::addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
    {
        _operation_groups[group_name].addWaitOperation(_primitives, semaphore_name, stage_mask);
    }
    void SyncObject::addWaitOperationToGroup(const std::string& group_name, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        _operation_groups[group_name].addWaitOperation(_primitives, semaphore_name, stage_mask, value);
    }
    void SyncObject::addSemaphoreForHostOperations(const std::string& group_name, const std::string& semaphore_name)
    {
        _operation_groups[group_name].getHostOperations().addSemaphore(_primitives, semaphore_name);
    }
    void SyncObject::signalSemaphore(const std::string& name, uint64_t value)
    {
        auto semaphore = _primitives.getSemaphore(name);

        VkSemaphoreSignalInfo signal_info{};
        signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signal_info.semaphore = semaphore;
        signal_info.value = _primitives.getTimelineOffset(name) + value;
        if (_primitives.getLogicalDevice()->vkSignalSemaphore(*_primitives.getLogicalDevice(), &signal_info) != VK_SUCCESS)
        {
            throw std::runtime_error("Couldn't signal semaphore: " + name);
        }
    }
    void SyncObject::waitSemaphore(const std::string& name, uint64_t value) const
    {
        auto semaphore = _primitives.getSemaphore(name);

        const uint64_t value_to_wait = _primitives.getTimelineOffset(name) + value;

        VkSemaphoreWaitInfo wait_info{};
        wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wait_info.pSemaphores = &semaphore;
        wait_info.semaphoreCount = 1;
        wait_info.pValues = &value_to_wait;
        if (_primitives.getLogicalDevice()->vkWaitSemaphores(*_primitives.getLogicalDevice(), &wait_info, UINT64_MAX) != VK_SUCCESS)
        {
            throw std::runtime_error("Couldn't wait semaphore: " + name);
        }
    }

    uint64_t RenderEngine::SyncObject::getSemaphoreValue(const std::string& name) const
    {
        auto semaphore = _primitives.getSemaphore(name);

        uint64_t value = 0;
        if (_primitives.getLogicalDevice()->vkGetSemaphoreCounterValue(*_primitives.getLogicalDevice(), semaphore, &value) != VK_SUCCESS)
        {
            throw std::runtime_error("Couldn't read semaphore value");
        }
        return value % _primitives.getTimelineWidth(name);
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
        VkSemaphore semaphore = _primitives.getSemaphore(name);
        for (auto& sync_operation : _operation_groups | std::views::values)
        {
            sync_operation.shiftTimelineSemaphoreValues(offset, semaphore);
        }
    }

    SyncObject::SyncObject(LogicalDevice& logical_device)
        : _primitives(logical_device)
        , _operation_groups{ }
    {}
}