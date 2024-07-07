#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/DataTransferScheduler.h>
#include <render_engine/memory/RefObj.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/rendergraph/Job.h>
#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/window/SwapChain.h>

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
        virtual void registerExectionContext(ExecutionContext&) = 0;

        virtual void accept(GraphVisitor&) = 0;

        virtual bool isActive() const = 0;

        bool isUsesTracking() const
        {
            return _uses_tracking.load(std::memory_order_relaxed);
        }

        void setUsesTracking(bool value)
        {
            _uses_tracking = value;
        }
    private:
        std::string _name;
        std::atomic_bool _uses_tracking{ false };
    };

    class NullNode final : public Node
    {
    public:
        NullNode(std::string name) : Node(std::move(name))
        {}
        ~NullNode() final {}
        void accept(GraphVisitor&) override {}

        void execute(ExecutionContext&, QueueSubmitTracker*) override {}
        void registerExectionContext(ExecutionContext&) override {};

        bool isActive() const override { return true; }
    };

    class RenderNode final : public Node
    {
    public:
        RenderNode(std::string name,
                   std::shared_ptr<CommandContext> command_context,
                   std::unique_ptr<AbstractRenderer> renderer)
            : Node(std::move(name))
            , _command_context(command_context)
            , _renderer(std::move(renderer))
        {
            setUsesTracking(true);
        }
        ~RenderNode() override = default;

        void accept(GraphVisitor& visitor) override;
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExectionContext(ExecutionContext&) override {};

        bool isActive() const override { return true; }
    private:
        using CommandBufferPerRenderTargetMap = std::map<uint32_t, VkCommandBuffer>;
        VkCommandBuffer createOrGetCommandBuffer(const ExecutionContext::PoolIndex& pool_index);

        std::shared_ptr<CommandContext> _command_context;
        std::unique_ptr<AbstractRenderer> _renderer;

        std::mutex _command_buffer_mutex;
        std::map<uint32_t, CommandBufferPerRenderTargetMap> _command_buffers;
    };

    class TransferNode final : public Node
    {
    public:
        TransferNode(std::string name, TransferEngine&& transfer_engine)
            : Node(std::move(name))
            , _transfer_engine(std::move(transfer_engine))
            , _scheduler()
        {}
        ~TransferNode() override = default;

        DataTransferScheduler& getScheduler() { return _scheduler; }

        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExectionContext(ExecutionContext&) override {};

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
        void registerExectionContext(ExecutionContext&) override {};

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
        void registerExectionContext(ExecutionContext& execution_context) override { _compute_task->registerExectionContext(execution_context); };
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
                    std::shared_ptr<CommandContext> command_context)
            : Node(std::move(name))
            , _swap_chain(swap_chain)
            , _command_context(command_context)
        {}
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExectionContext(ExecutionContext&) override {};

        void accept(GraphVisitor& visitor) final;
        bool isActive() const final { return true; }
    private:
        SwapChain& _swap_chain;
        std::shared_ptr<CommandContext> _command_context;

    };

    class CpuNode final : public Node
    {
    public:
        class ICpuTask
        {
        public:
            virtual ~ICpuTask() = default;
            virtual void run(ExecutionContext& execution_context) = 0;
            virtual bool isActive() const = 0;
            virtual void registerExectionContext(ExecutionContext&) = 0;
        };
        CpuNode(std::string name, std::unique_ptr<ICpuTask> cpu_task)
            : Node(std::move(name))
            , _cpu_task(std::move(cpu_task))
        {}
        void execute(ExecutionContext& execution_context, QueueSubmitTracker* queue_tracker) override;
        void registerExectionContext(ExecutionContext& execution_context) override { _cpu_task->registerExectionContext(execution_context); };

        void accept(GraphVisitor& visitor) final;
        bool isActive() const final { return _cpu_task->isActive(); }
    private:
        std::unique_ptr<ICpuTask> _cpu_task;
    };
}