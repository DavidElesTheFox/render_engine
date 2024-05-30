#include <render_engine/synchronization/SyncObject.h>

#include <ranges>
#include <stdexcept>

namespace RenderEngine
{
    uint32_t RenderEngine::SyncObject::SharedOperations::waitAnyOfSemaphores(std::string semaphore_names, uint64_t value)&&
    {
        std::vector<VkSemaphore> semaphores;
        std::optional<uint64_t> current_value;
        std::optional<uint64_t> timeline_offset;
        LogicalDevice& logical_device = _sync_objects[0]->getPrimitives().getLogicalDevice();

        for (auto& sync_object : _sync_objects)
        {
            if (sync_object->getPrimitives().hasSemaphore(semaphore_names) == false)
            {
                continue;
            }
            semaphores.push_back(sync_object->getPrimitives().getSemaphore(semaphore_names));
            if (timeline_offset == std::nullopt)
            {
                timeline_offset = sync_object->getPrimitives().getTimelineOffset(semaphore_names);
            }
            else
            {
                assert(timeline_offset == sync_object->getPrimitives().getTimelineOffset(semaphore_names));
            }
            if (current_value == std::nullopt)
            {
                current_value = sync_object->getSemaphoreValue(semaphore_names);
            }
            else
            {
                assert(current_value == sync_object->getSemaphoreValue(semaphore_names));
            }
        }
        if (timeline_offset == std::nullopt)
        {
            throw std::runtime_error("Shared operation should wait a semaphore that is not defined in the set of sync objects.");
        }
        const uint64_t value_to_wait = *timeline_offset + value;

        VkSemaphoreWaitInfo wait_info{};
        wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wait_info.pSemaphores = semaphores.data();
        wait_info.semaphoreCount = static_cast<uint32_t>(semaphores.size());
        wait_info.pValues = &value_to_wait;
        wait_info.flags = VK_SEMAPHORE_WAIT_ANY_BIT;

        if (logical_device->vkWaitSemaphores(*logical_device, &wait_info, UINT64_MAX) != VK_SUCCESS)
        {
            throw std::runtime_error("Couldn't wait semaphore: " + semaphore_names);
        }

        for (uint32_t i = 0; i < _sync_objects.size(); ++i)
        {
            const auto& sync_object = _sync_objects[i];
            if (sync_object->getPrimitives().hasSemaphore(semaphore_names) == false)
            {
                continue;
            }
            if (sync_object->getSemaphoreValue(semaphore_names) == value_to_wait)
            {
                return i;
            }
        }
        assert(false && "Wait was successful but no semaphore was found with the wait value. Should never happen");
        return 0;
    }

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

    std::pair<VkResult, uint32_t> SyncObject::acquireNextSwapChainImage(LogicalDevice& logical_device, VkSwapchainKHR swap_chain, const std::string& semaphore_name) const
    {
        uint32_t image_index = 0;
        auto call_result = logical_device->vkAcquireNextImageKHR(*logical_device,
                                                                 swap_chain,
                                                                 UINT64_MAX,
                                                                 getPrimitives().getSemaphore(semaphore_name),
                                                                 VK_NULL_HANDLE,
                                                                 &image_index);
        return { call_result, image_index };
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