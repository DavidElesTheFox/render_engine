#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/rendergraph/Graph.h>
#include <render_engine/window/IWindow.h>

#pragma warning(push, 0)
#include <taskflow/taskflow.hpp>
#pragma warning(pop)
namespace RenderEngine
{
    class ParallelRenderEngine
    {
    public:
        class RenderGraphBuilder
        {
        public:
            class SemaphoreValue
            {
            public:
                explicit SemaphoreValue(uint64_t value);
                explicit operator uint64_t();
            private:
                uint64_t _value{ 0 };
            };
            class GpuLinkBuilder
            {
            public:
                explicit GpuLinkBuilder(RenderGraph::Link& link)
                    : _link(link)
                {}
                GpuLinkBuilder&& attachGpuLink(VkPipelineStageFlagBits2 stage_mask)&&;
                GpuLinkBuilder&& attachGpuLink(VkPipelineStageFlagBits2 stage_mask, uint64_t value)&&;
                GpuLinkBuilder&& attachGpuLink(const std::string& signaled_semaphore_name, VkPipelineStageFlagBits2 wait_mask);
                GpuLinkBuilder&& attachGpuLink(const std::string& signaled_semaphore_name, SemaphoreValue value);
            private:
                RenderGraph::Link& _link;
            };
            friend class ParallelRenderEngine;
            ~RenderGraphBuilder() = default;

            void addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer);
            void addTransferNode(std::string name);
            void addComputeNode(std::string name, std::unique_ptr<RenderGraph::ComputeNode::IComputeTask> task);
            void addCpuNode(std::string name, std::unique_ptr<RenderGraph::CpuNode::ICpuTask> task);
            void addPresentNode(std::string name, SwapChain& swap_chain);

            GpuLinkBuilder addCpuSyncLink(const std::string& from, const std::string& to);
            GpuLinkBuilder addCpuAsyncLink(const std::string& from, const std::string& to);

            std::unique_ptr<RenderGraph::Graph> reset(std::string new_name)
            {
                auto result = std::move(_graph);
                _graph = std::make_unique<RenderGraph::Graph>(std::move(new_name));
                return result;
            }
        private:
            explicit RenderGraphBuilder(std::string name, ParallelRenderEngine& render_engine);

            std::unique_ptr<RenderGraph::Graph> _graph;
            ParallelRenderEngine& _render_engine;
        };

        RenderGraphBuilder createRenderGraphBuilder(std::string graph_name);
        void setRenderGraph(std::unique_ptr<RenderGraph::Graph> render_graph);

        void render();
    private:
        struct RenderingProcess
        {
            RenderingProcess() = default;

            RenderingProcess(RenderingProcess&& o) noexcept = delete;
            RenderingProcess(const RenderingProcess&) noexcept = delete;

            RenderingProcess& operator=(RenderingProcess&&) noexcept = delete;
            RenderingProcess& operator=(const RenderingProcess&) noexcept = delete;

            RenderGraph::Job::ExecutionContext execution_contexts;
            tf::Taskflow task_flow;
            tf::Executor executor;
            tf::Future<void> calling_token;
        };
        Device& _device;
        std::shared_ptr<CommandContext> _render_context;
        std::shared_ptr<CommandContext> _present_context;
        std::shared_ptr<SingleShotCommandContext> _transfer_context;

        std::unique_ptr<RenderGraph::Graph> _skeleton;
        std::unique_ptr<GpuResourceManager> _gpu_resource_manager;
        std::vector<std::unique_ptr<RenderingProcess>> _rendering_processes;
        uint32_t _render_call_count{ 0 };
    };
}