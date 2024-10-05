#pragma once

#include <volk.h>

#include <render_engine/DeviceLookup.h>
#include <render_engine/LogicalDevice.h>
#include <render_engine/memory/RefObj.h>
#include <render_engine/QueueLoadBallancer.h>
#include <render_engine/window/SwapChain.h>


#include <cassert>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace RenderEngine
{
    class SyncOperations;

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
    class SingleShotCommandBufferFactory
    {
        struct CreationToken
        {};
    public:
        static std::shared_ptr<SingleShotCommandBufferFactory> create(RefObj<VulkanQueue> queue)
        {
            return std::make_shared<SingleShotCommandBufferFactory>(std::move(queue),
                                                                    CreationToken{});
        }
        SingleShotCommandBufferFactory(RefObj<VulkanQueue> queue, CreationToken);

        SingleShotCommandBufferFactory(SingleShotCommandBufferFactory&&) noexcept = delete;
        SingleShotCommandBufferFactory(const SingleShotCommandBufferFactory&) = delete;

        SingleShotCommandBufferFactory& operator=(SingleShotCommandBufferFactory&&) noexcept = delete;
        SingleShotCommandBufferFactory& operator=(const SingleShotCommandBufferFactory&) = delete;

        ~SingleShotCommandBufferFactory();

        VkCommandBuffer createCommandBuffer(uint32_t tray_index);

        VulkanQueue& getQueue() { return *_vulkan_queue; }
        const VulkanQueue& getQueue() const { return *_vulkan_queue; }

    private:
        class Tray;

        RefObj<VulkanQueue> _vulkan_queue;
        std::unordered_map<uint32_t, std::unique_ptr<Tray>> _trays;
        mutable std::mutex _trays_mutex;
    };

    class CommandBufferFactory
    {
        struct CreationToken
        {};
    public:
        static std::shared_ptr<CommandBufferFactory> create(RefObj<VulkanQueue> queue,
                                                            uint32_t num_of_pool_per_tray)
        {
            return std::make_shared<CommandBufferFactory>(std::move(queue),
                                                          num_of_pool_per_tray,
                                                          CreationToken{});
        }
        CommandBufferFactory(RefObj<VulkanQueue> queue,
                             uint32_t num_of_pools_per_tray,
                             CreationToken);

        CommandBufferFactory(CommandBufferFactory&&) noexcept = delete;
        CommandBufferFactory(const CommandBufferFactory&) = delete;

        CommandBufferFactory& operator=(CommandBufferFactory&&) noexcept = delete;
        CommandBufferFactory& operator=(const CommandBufferFactory&) = delete;

        ~CommandBufferFactory();
        VkCommandBuffer createCommandBuffer(uint32_t tray_index, uint32_t pool_index);
        std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count, uint32_t tray_index, uint32_t pool_index);

        VulkanQueue& getQueue() { return *_vulkan_queue; }
        const VulkanQueue& getQueue() const { return *_vulkan_queue; }

    private:
        class Tray;

        RefObj<VulkanQueue> _vulkan_queue;
        uint32_t _num_of_pools_per_tray{ 0 };
        std::unordered_map<uint32_t, std::unique_ptr<Tray>> _trays;
        mutable std::mutex _trays_mutex;
    };
    class CommandBufferContext
    {
    public:

        CommandBufferContext(RefObj<VulkanQueue> queue,
                             uint32_t num_of_pool_per_tray)
            : _factory(CommandBufferFactory::create(queue, num_of_pool_per_tray))
            , _single_shot_factory(SingleShotCommandBufferFactory::create(queue))
        {}

        const std::shared_ptr<CommandBufferFactory>& getFactory() const { return _factory; }
        std::shared_ptr<CommandBufferFactory>& getFactory() { return _factory; }

        const std::shared_ptr<SingleShotCommandBufferFactory>& getSingleShotFactory() const { return _single_shot_factory; }
        std::shared_ptr<SingleShotCommandBufferFactory>& getSingleShotFactory() { return _single_shot_factory; }

        VulkanQueue& getQueue() { return _factory->getQueue(); }
        const VulkanQueue& getQueue() const { return _factory->getQueue(); }
    private:
        std::shared_ptr<CommandBufferFactory> _factory;
        std::shared_ptr<SingleShotCommandBufferFactory> _single_shot_factory;
    };




}
