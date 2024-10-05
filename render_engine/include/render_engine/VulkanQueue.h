#pragma once
#include <render_engine/DeviceLookup.h>
#include <render_engine/LogicalDevice.h>
#include <render_engine/QueueLoadBallancer.h>

namespace RenderEngine
{
    class SyncOperations;
    class SwapChain;

    class VulkanQueue
    {
    public:
        VulkanQueue(LogicalDevice* logical_device,
                    uint32_t queue_family_index,
                    uint32_t queue_count,
                    DeviceLookup::QueueFamilyInfo queue_family_info);

        VulkanQueue(VulkanQueue&& o) noexcept = delete;
        VulkanQueue(const VulkanQueue& o) noexcept = delete;

        VulkanQueue& operator=(VulkanQueue&& o) noexcept = delete;
        VulkanQueue& operator=(const VulkanQueue& o) noexcept = delete;
        QueueLoadBalancer& getLoadBalancer() { return _queue_load_balancer; }
        virtual ~VulkanQueue() = default;
        uint32_t getQueueFamilyIndex() const { return _queue_family_index; }
        LogicalDevice& getLogicalDevice() { return *_logical_device; }
        LogicalDevice& getLogicalDevice() const { return *_logical_device; }

        bool isPipelineStageSupported(VkPipelineStageFlags2 pipeline_stage) const;

        bool isCompatibleWith(VulkanQueue& o) const
        {
            return o._queue_family_index == _queue_family_index;
        }

        GuardedQueue getVulkanGuardedQueue();

        void queueSubmit(VkSubmitInfo2&& submit_info,
                         const SyncOperations& sync_operations,
                         VkFence fence);
        void queuePresent(VkPresentInfoKHR&& present_info,
                          const SyncOperations& sync_operations,
                          SwapChain& swap_chain);


    private:

        LogicalDevice* _logical_device{ nullptr };
        uint32_t _queue_family_index{ 0 };
        DeviceLookup::QueueFamilyInfo _queue_family_info;

        QueueLoadBalancer _queue_load_balancer;

    };
}