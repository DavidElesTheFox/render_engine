#pragma once

#include <render_engine/LogicalDevice.h>

#include <cassert>
#include <list>
#include <mutex>

#include <volk.h>

namespace RenderEngine
{
    class GuardedQueue
    {
    public:
        GuardedQueue(VkQueue queue, std::unique_lock<std::mutex>&& lock)
            : _queue(queue)
            , _lock(std::move(lock))
        {}
        GuardedQueue(const GuardedQueue&) = delete;
        GuardedQueue(GuardedQueue&&) = default;

        GuardedQueue& operator=(const GuardedQueue&) = delete;
        GuardedQueue& operator=(GuardedQueue&&) = default;

        void lock() { _lock.lock(); }
        void unlock() { _lock.unlock(); }
        VkQueue getQueue()
        {
            assert(_lock.owns_lock());
            return _queue;
        }
    private:
        VkQueue _queue{ VK_NULL_HANDLE };
        std::unique_lock<std::mutex> _lock;
    };

    /*
    * TODO: The abstraction is wrong. The problems:
    *  - Maybe this queue family is used for all purpose (render, compute, transfer). In this case the queues should be distributed based on this dimension
    *  - Previous problem is not a problem only when different queue families are used
    *  - No real benefits due to synchronization: I.e.: Imagine the simple situation: SceneRender->UI->Present. Here using 2 different queues for the two render passes
    *    makes no sense because it is already synchronized. Also another issue can be that the synchronization needs to be done with timeline semaphores due to
    *    queue<->queue synchronization.
    *
    * A better approach would be 2 dimensional: 1 Frame: 1 render queue, 2 transfer queue (upload/download), multiple compute queues
    */
    class QueueLoadBalancer
    {
    public:
        explicit QueueLoadBalancer(LogicalDevice& logical_device, uint32_t queue_family_index, uint32_t queue_count);
        QueueLoadBalancer(const QueueLoadBalancer&) = delete;
        QueueLoadBalancer(QueueLoadBalancer&&) = delete;

        QueueLoadBalancer& operator=(const QueueLoadBalancer&) = delete;
        QueueLoadBalancer& operator=(QueueLoadBalancer&&) = delete;
        GuardedQueue getQueue();
    private:
        struct QueueData
        {
            VkQueue queue{ VK_NULL_HANDLE };
            std::mutex access_mutex;
            uint32_t access_count{ 0 };

            explicit QueueData(VkQueue queue)
                : queue(queue)
            {}
            QueueData(const QueueData&) = delete;
            QueueData(QueueData&&) = delete;

            QueueData& operator=(const QueueData&) = delete;
            QueueData& operator=(QueueData&&) = delete;
        };
        std::list<QueueData> _queue_map;
        std::mutex _queue_map_mutex;
    };
}