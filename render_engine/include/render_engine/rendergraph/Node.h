#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/DataTransferScheduler.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/rendergraph/Job.h>
#include <render_engine/synchronization/SyncOperations.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/window/SwapChain.h>

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

        virtual std::unique_ptr<Job> createJob(const SyncOperations& in_operations) = 0;

        virtual void accept(GraphVisitor&) = 0;

        virtual bool isActive() const = 0;
    private:
        std::string _name;
    };

    class RenderNode final : public Node
    {
    public:
        RenderNode(std::string name,
                   std::shared_ptr<CommandContext> command_context,
                   std::unique_ptr<AbstractRenderer> renderer,
                   bool enable_tracking)
            : Node(std::move(name))
            , _command_context(command_context)
            , _renderer(std::move(renderer))
            , _enable_tracking(enable_tracking)
        {}
        ~RenderNode() override = default;

        void accept(GraphVisitor& visitor) override;

        std::unique_ptr<Job> createJob(const SyncOperations& in_operations) override;
        bool isActive() const override { return true; }
    private:
        std::shared_ptr<CommandContext> _command_context;
        std::unique_ptr<AbstractRenderer> _renderer;
        bool _enable_tracking{ false };
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
        std::unique_ptr<Job> createJob(const SyncOperations& in_operations) override;

        void accept(GraphVisitor& visitor) override;
        bool isActive() const override;
    private:
        TransferEngine _transfer_engine;
        DataTransferScheduler _scheduler;
    };

    class ComputeNode final : public Node
    {
    public:
        class IComputeTask
        {
        public:
            virtual ~IComputeTask() = default;
            virtual bool isActive() const = 0;
            virtual void run(const SyncOperations& in_operations, Job::ExecutionContext& execution_context) = 0;
        };
        ComputeNode(std::string name,
                    std::unique_ptr<IComputeTask> compute_task)
            : Node(std::move(name))
            , _compute_task(std::move(compute_task))
        {}

        ~ComputeNode() override = default;
        std::unique_ptr<Job> createJob(const SyncOperations& in_operations) override;
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
        std::unique_ptr<Job> createJob(const SyncOperations& in_operations) override;

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
            virtual void run(const SyncOperations& in_operations, Job::ExecutionContext& execution_context) = 0;
            virtual bool isActive() const = 0;
        };
        CpuNode(std::string name, std::unique_ptr<ICpuTask> cpu_task)
            : Node(std::move(name))
            , _cpu_task(std::move(cpu_task))
        {}
        std::unique_ptr<Job> createJob(const SyncOperations& in_operations) override;

        void accept(GraphVisitor& visitor) final;
        bool isActive() const final { return _cpu_task->isActive(); }
    private:
        std::unique_ptr<ICpuTask> _cpu_task;
    };
}