#include <render_engine/QueueLoadBallancer.h>

#include <render_engine/LogicalDevice.h>

namespace RenderEngine
{
    QueueLoadBalancer::QueueLoadBalancer(LogicalDevice& logical_device, uint32_t queue_family_index, uint32_t queue_count)
    {
        for (uint32_t i = 0; i < queue_count; ++i)
        {
            VkQueue queue;
            logical_device->vkGetDeviceQueue(*logical_device, queue_family_index, i, &queue);

            _queue_map.emplace_back(queue);
        }
    }
    GuardedQueue QueueLoadBalancer::getQueue()
    {
        std::lock_guard lock{ _queue_map_mutex };
        auto it = std::min_element(_queue_map.begin(), _queue_map.end(),
                                   [](auto& a, auto& b) { return a.access_count < b.access_count; });
        it->access_count++;
        return GuardedQueue{ it->queue,  std::unique_lock{ it->access_mutex } };
    }
}