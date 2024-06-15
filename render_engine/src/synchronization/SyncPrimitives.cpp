#include <render_engine/synchronization/SyncPrimitives.h>


#include <cassert>
#include <ranges>
#include <stdexcept>


namespace RenderEngine
{

    SyncPrimitives::~SyncPrimitives()
    {
        for (VkSemaphore semaphore : _semaphore_map | std::views::values)
        {
            getLogicalDevice()->vkDestroySemaphore(*getLogicalDevice(), semaphore, nullptr);
        }
    }
    SyncPrimitives::SyncPrimitives(SyncPrimitives&& o)
        : _semaphore_map(std::move(o._semaphore_map))
        , _timeline_data(std::move(o._timeline_data))
        , _logical_device(o._logical_device)
    {
        o._logical_device = nullptr;
        o._semaphore_map.clear();
        o._timeline_data.clear();
    }
    SyncPrimitives& SyncPrimitives::operator=(SyncPrimitives&& o)
    {
        std::swap(o._semaphore_map, _semaphore_map);
        std::swap(o._timeline_data, _timeline_data);
        std::swap(o._logical_device, _logical_device);
        return *this;
    }
    void SyncPrimitives::createSemaphore(std::string name)
    {
        VkSemaphoreCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphore semaphore;
        if (getLogicalDevice()->vkCreateSemaphore(*getLogicalDevice(), &create_info, nullptr, &semaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create semaphore");
        }
        _semaphore_map.emplace(std::move(name), semaphore);
    }
    void SyncPrimitives::createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width)
    {
        VkSemaphoreTypeCreateInfo type_info{};
        type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        type_info.initialValue = initial_value;

        VkSemaphoreCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        create_info.pNext = &type_info;
        VkSemaphore semaphore;
        if (getLogicalDevice()->vkCreateSemaphore(*getLogicalDevice(), &create_info, nullptr, &semaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create semaphore");
        }

        _semaphore_map.emplace(name, semaphore);
        _timeline_data.emplace(std::move(name),
                               TimelineSemaphoreData{ .timeline_width = timeline_width, .initial_value = initial_value });
    }
    uint64_t SyncPrimitives::stepTimeline(const std::string& name)
    {
        auto& timeline_data = _timeline_data.at(name);
        // TODO recreate Timeline semaphore when overflow happened. initial value is stored so probably it is a small fix.
        assert(timeline_data.timeline_offset < timeline_data.timeline_offset + timeline_data.timeline_width
               && "Overflow happened. Semaphore needs to be recreated to be able to continue");

        timeline_data.timeline_offset += timeline_data.timeline_width;
        return timeline_data.timeline_width;
    }
}