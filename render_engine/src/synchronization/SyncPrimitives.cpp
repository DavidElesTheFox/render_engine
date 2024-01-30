#include <render_engine/synchronization/SyncPrimitives.h>

#include <cassert>
#include <ranges>
#include <stdexcept>

namespace RenderEngine
{
    SyncPrimitives SyncPrimitives::CreateWithFence(VkDevice logical_device, VkFenceCreateFlags flags)
    {
        SyncPrimitives synchronization_primitives{ logical_device };
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
    SyncPrimitives::~SyncPrimitives()
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
    SyncPrimitives::SyncPrimitives(SyncPrimitives&& o)
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
    SyncPrimitives& SyncPrimitives::operator=(SyncPrimitives&& o)
    {
        std::swap(o._semaphore_map, _semaphore_map);
        std::swap(o._timeline_data, _timeline_data);
        std::swap(o._logical_device, _logical_device);
        std::swap(o._fence, _fence);
        return *this;
    }
    void SyncPrimitives::createSemaphore(std::string name)
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
        if (vkCreateSemaphore(_logical_device, &create_info, nullptr, &semaphore) != VK_SUCCESS)
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