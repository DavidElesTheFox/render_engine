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



    class AbstractCommandContext
    {
    public:
        virtual ~AbstractCommandContext() = default;
        uint32_t getQueueFamilyIndex() const { return _queue_family_index; }
        LogicalDevice& getLogicalDevice() { return *_logical_device; }
        LogicalDevice& getLogicalDevice() const { return *_logical_device; }

        bool isPipelineStageSupported(VkPipelineStageFlags2 pipeline_stage) const;

        bool isCompatibleWith(AbstractCommandContext& o) const
        {
            return o._queue_family_index == _queue_family_index;
        }

        GuardedQueue getQueue();

        void queueSubmit(VkSubmitInfo2&& submit_info,
                         const SyncOperations& sync_operations,
                         VkFence fence);
        void queuePresent(VkPresentInfoKHR&& present_info,
                          const SyncOperations& sync_operations,
                          SwapChain& swap_chain);
    protected:
        AbstractCommandContext(LogicalDevice& logical_device,
                               uint32_t queue_family_index,
                               DeviceLookup::QueueFamilyInfo queue_family_info,
                               RefObj<QueueLoadBalancer> queue_load_balancer);

        AbstractCommandContext(AbstractCommandContext&& o) noexcept = delete;
        AbstractCommandContext(const AbstractCommandContext& o) noexcept = delete;

        AbstractCommandContext& operator=(AbstractCommandContext&& o) noexcept = delete;
        AbstractCommandContext& operator=(const AbstractCommandContext& o) noexcept = delete;
        QueueLoadBalancer& getLoadBalancer() { return *_queue_load_balancer; }
    private:

        LogicalDevice* _logical_device{ nullptr };
        uint32_t _queue_family_index{ 0 };
        DeviceLookup::QueueFamilyInfo _queue_family_info;

        RefObj<QueueLoadBalancer> _queue_load_balancer;

    };

    class SingleShotCommandContext : public std::enable_shared_from_this<SingleShotCommandContext>,
        public AbstractCommandContext
    {
        struct CreationToken
        {};
    public:
        static std::shared_ptr<SingleShotCommandContext> create(LogicalDevice& logical_device,
                                                                uint32_t queue_family_index,
                                                                DeviceLookup::QueueFamilyInfo queue_family_info,
                                                                RefObj<QueueLoadBalancer> queue_load_balancer)
        {
            return std::make_shared<SingleShotCommandContext>(logical_device,
                                                              queue_family_index,
                                                              std::move(queue_family_info),
                                                              std::move(queue_load_balancer),
                                                              CreationToken{});
        }
        SingleShotCommandContext(LogicalDevice& logical_device,
                                 uint32_t queue_family_index,
                                 DeviceLookup::QueueFamilyInfo queue_family_info,
                                 RefObj<QueueLoadBalancer> queue_load_balancer,
                                 CreationToken);

        SingleShotCommandContext(SingleShotCommandContext&&) noexcept = delete;
        SingleShotCommandContext(const SingleShotCommandContext&) = delete;

        SingleShotCommandContext& operator=(SingleShotCommandContext&&) noexcept = delete;
        SingleShotCommandContext& operator=(const SingleShotCommandContext&) = delete;

        ~SingleShotCommandContext() override;
        VkCommandBuffer createCommandBuffer(uint32_t tray_index);


        std::weak_ptr<SingleShotCommandContext> getWeakReference() { return shared_from_this(); }

    private:
        class Tray;

        using std::enable_shared_from_this<SingleShotCommandContext>::shared_from_this;
        using std::enable_shared_from_this<SingleShotCommandContext>::weak_from_this;


        std::unordered_map<uint32_t, std::unique_ptr<Tray>> _trays;
        mutable std::mutex _trays_mutex;
    };

    class CommandContext : public AbstractCommandContext
    {
        struct CreationToken
        {};
    public:
        static std::shared_ptr<CommandContext> create(LogicalDevice& logical_device,
                                                      uint32_t queue_family_index,
                                                      DeviceLookup::QueueFamilyInfo queue_family_info,
                                                      RefObj<QueueLoadBalancer> queue_load_balancer,
                                                      uint32_t num_of_pool_per_tray)
        {
            return std::make_shared<CommandContext>(logical_device,
                                                    queue_family_index,
                                                    std::move(queue_family_info),
                                                    std::move(queue_load_balancer),
                                                    num_of_pool_per_tray,
                                                    CreationToken{});
        }
        CommandContext(LogicalDevice& logical_device,
                       uint32_t queue_family_index,
                       DeviceLookup::QueueFamilyInfo queue_family_info,
                       RefObj<QueueLoadBalancer> queue_load_balancer,
                       uint32_t num_of_pools_per_tray,
                       CreationToken);

        CommandContext(CommandContext&&) noexcept = delete;
        CommandContext(const CommandContext&) = delete;

        CommandContext& operator=(CommandContext&&) noexcept = delete;
        CommandContext& operator=(const CommandContext&) = delete;

        ~CommandContext() override;
        VkCommandBuffer createCommandBuffer(uint32_t tray_index, uint32_t pool_index);
        std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count, uint32_t tray_index, uint32_t pool_index);


    private:
        class Tray;

        uint32_t _num_of_pools_per_tray{ 0 };
        std::unordered_map<uint32_t, std::unique_ptr<Tray>> _trays;
        mutable std::mutex _trays_mutex;
    };


}
