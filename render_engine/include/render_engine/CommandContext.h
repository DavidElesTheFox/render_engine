#pragma once

#include <volk.h>

#include <render_engine/DeviceLookup.h>
#include <render_engine/LogicalDevice.h>
#include <render_engine/memory/RefObj.h>
#include <render_engine/QueueLoadBallancer.h>


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
        void queuePresent(VkPresentInfoKHR&& present_info, const SyncOperations& sync_operations);
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
        VkCommandBuffer createCommandBuffer();


        std::weak_ptr<SingleShotCommandContext> getWeakReference() { return shared_from_this(); }

    private:
        class ThreadData;

        using std::enable_shared_from_this<SingleShotCommandContext>::shared_from_this;
        using std::enable_shared_from_this<SingleShotCommandContext>::weak_from_this;


        std::unordered_map<std::thread::id, std::unique_ptr<ThreadData>> _thread_data;
        mutable std::mutex _thread_data_mutex;
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
                                                      uint32_t back_buffer_size)
        {
            return std::make_shared<CommandContext>(logical_device,
                                                    queue_family_index,
                                                    std::move(queue_family_info),
                                                    std::move(queue_load_balancer),
                                                    back_buffer_size,
                                                    CreationToken{});
        }
        CommandContext(LogicalDevice& logical_device,
                       uint32_t queue_family_index,
                       DeviceLookup::QueueFamilyInfo queue_family_info,
                       RefObj<QueueLoadBalancer> queue_load_balancer,
                       uint32_t back_buffer_size,
                       CreationToken);

        CommandContext(CommandContext&&) noexcept = delete;
        CommandContext(const CommandContext&) = delete;

        CommandContext& operator=(CommandContext&&) noexcept = delete;
        CommandContext& operator=(const CommandContext&) = delete;

        ~CommandContext() override;
        VkCommandBuffer createCommandBuffer(uint32_t render_target_image_index);
        std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count, uint32_t render_target_image_index);


    private:
        class ThreadData;

        uint32_t _back_buffer_size{ 0 };

        std::unordered_map<std::thread::id, std::unique_ptr<ThreadData>> _thread_data;
        mutable std::mutex _thread_data_mutex;
    };


}
