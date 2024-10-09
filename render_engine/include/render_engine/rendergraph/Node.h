#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/DataTransferScheduler.h>
#include <render_engine/memory/RefObj.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/rendergraph/ExecutionContext.h>
#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/window/SwapChain.h>

#include <deque>
#include <map>
#include <memory>
#include <string>

namespace RenderEngine::RenderGraph
{
    class GraphVisitor;
    class Graph;
    class Node
    {
    public:
        explicit Node(std::string name)
            : _name(std::move(name))
        {}
        virtual ~Node() = default;

        Node(Node&&) = delete;
        Node(const Node&) = delete;

        Node& operator=(Node&&) = delete;
        Node& operator=(const Node&) = delete;

        const std::string& getName() const
        {
            return _name;
        }

        virtual void execute(ExecutionContext&, QueueSubmitTracker*) = 0;
        virtual void registerExecutionContext(ExecutionContext&) = 0;

        virtual void accept(GraphVisitor&) = 0;

        virtual bool isActive() const = 0;

    private:
        std::string _name;
    };

    class NullNode final : public Node
    {
    public:
        NullNode(std::string name) : Node(std::move(name))
        {}
        ~NullNode() final {}
        void accept(GraphVisitor&) override {}

        void execute(ExecutionContext&, QueueSubmitTracker*) override {}
        void registerExecutionContext(ExecutionContext&) override {};

        bool isActive() const override { return true; }
    };

    class BaseGpuNode : public Node
    {
    public:
        BaseGpuNode(std::string name,
                    std::shared_ptr<CommandBufferFactory> command_buffer_factory)
            : Node(std::move(name))
            , _command_buffer_factory(command_buffer_factory)
        {}
        ~BaseGpuNode() override = default;

        VkCommandBuffer createOrGetCommandBuffer(const ExecutionContext::PoolIndex& pool_index);
        VulkanQueue& getQueue() { return _command_buffer_factory->getQueue(); }
    private:
        using CommandBufferPerRenderTargetMap = std::map<uint32_t, VkCommandBuffer>;

        std::shared_ptr<CommandBufferFactory> _command_buffer_factory;

        std::mutex _command_buffer_mutex;
        std::map<uint32_t, CommandBufferPerRenderTargetMap> _command_buffers;
    };

    class RenderNode final : public BaseGpuNode
    {
    public:
        RenderNode(std::string name,
                   std::shared_ptr<CommandBufferFactory> command_buffer_factory,
                   std::unique_ptr<AbstractRenderer> renderer)
            : BaseGpuNode(std::move(name), command_buffer_factory)
            , _renderer(std::move(renderer))
        {}
        ~RenderNode() override = default;

        void accept(GraphVisitor& visitor) override;
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExecutionContext(ExecutionContext&) override {};

        bool isActive() const override { return true; }
    private:
        std::unique_ptr<AbstractRenderer> _renderer;
    };

    class TransferNode final : public Node
    {
    public:
        TransferNode(std::string name, TransferEngine transfer_engine)
            : Node(std::move(name))
            , _transfer_engine(std::move(transfer_engine))
            , _scheduler()
        {}
        ~TransferNode() override = default;

        DataTransferScheduler& getScheduler() { return _scheduler; }

        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExecutionContext(ExecutionContext&) override {};

        void accept(GraphVisitor& visitor) override;
        bool isActive() const override;
    private:
        TransferEngine _transfer_engine;
        DataTransferScheduler _scheduler;
    };

    class DeviceSynchronizeNode final : public Node
    {
    public:
        DeviceSynchronizeNode(std::string name, Device& device)
            : Node(std::move(name))
            , _device(device)
        {}
        ~DeviceSynchronizeNode() override = default;

        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExecutionContext(ExecutionContext&) override {};

        void accept(GraphVisitor& visitor) override;
        bool isActive() const override;
    private:
        RefObj<Device> _device;
    };

    class ComputeNode final : public Node
    {
    public:
        class IComputeTask
        {
        public:
            virtual ~IComputeTask() = default;
            virtual bool isActive() const = 0;
            virtual void run(ExecutionContext& execution_context) = 0;
            virtual void registerExectionContext(ExecutionContext&) = 0;

        };
        ComputeNode(std::string name,
                    std::unique_ptr<IComputeTask> compute_task)
            : Node(std::move(name))
            , _compute_task(std::move(compute_task))
        {}

        ~ComputeNode() override = default;
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExecutionContext(ExecutionContext& execution_context) override { _compute_task->registerExectionContext(execution_context); };
        bool isActive() const override { return _compute_task->isActive(); }

        void accept(GraphVisitor& visitor) override;
    private:
        std::unique_ptr<IComputeTask> _compute_task;
    };

    class PresentNode final : public Node
    {
    public:
        PresentNode(std::string name,
                    SwapChain& swap_chain,
                    std::shared_ptr<CommandBufferFactory> command_buffer_factory)
            : Node(std::move(name))
            , _swap_chain(swap_chain)
            , _command_buffer_factory(command_buffer_factory)
        {}
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExecutionContext(ExecutionContext&) override {};

        void accept(GraphVisitor& visitor) final;
        bool isActive() const final { return true; }
    private:
        [[nodiscard]]
        bool waitAndUpdateFrameNumber(uint64_t frame_number, const std::chrono::milliseconds& timeout);

        SwapChain& _swap_chain;
        std::shared_ptr<CommandBufferFactory> _command_buffer_factory;

        std::mutex _frame_continuity_mutex;
        std::condition_variable _frame_continuity_condition;
        uint64_t _current_frame_number{ 0 };
    };

    class CpuNode : public BaseGpuNode
    {
    public:
        class ICpuTask
        {
        public:
            virtual ~ICpuTask() = default;
            virtual void run(CpuNode& self, ExecutionContext& execution_context) = 0;
            virtual bool isActive() const = 0;
            virtual void registerExecutionContext(ExecutionContext&) = 0;
        };
        CpuNode(std::string name, std::unique_ptr<ICpuTask> cpu_task, std::shared_ptr<CommandBufferFactory> command_buffer_factory)
            : BaseGpuNode(std::move(name), command_buffer_factory)
            , _cpu_task(std::move(cpu_task))
        {}
        ~CpuNode() override = default;
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) final;
        void registerExecutionContext(ExecutionContext& execution_context) final { _cpu_task->registerExecutionContext(execution_context); };

        void accept(GraphVisitor& visitor) final;
        bool isActive() const final { return _cpu_task->isActive(); }
    protected:
        template<typename T>
            requires std::derived_from<T, ICpuTask>
        T& getTaskAs() { return static_cast<T&>(*_cpu_task); }

        template<typename T>
            requires std::derived_from<T, ICpuTask>
        const T& getTaskAs() const { return static_cast<const T&>(*_cpu_task); }
    private:
        std::unique_ptr<ICpuTask> _cpu_task;
    };

}