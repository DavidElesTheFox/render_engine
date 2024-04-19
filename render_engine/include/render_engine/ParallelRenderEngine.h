#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/rendergraph/Graph.h>
#include <render_engine/rendergraph/RenderGraphBuilder.h>
#include <render_engine/window/IWindow.h>

#pragma warning(push, 0)
#include <taskflow/taskflow.hpp>
#pragma warning(pop)

#include <tuple>
#include <format>
namespace RenderEngine
{
    class ParallelRenderEngine
    {
    public:

        ParallelRenderEngine(Device& device,
                             std::shared_ptr<SingleShotCommandContext>&& transfer_context,
                             std::shared_ptr<CommandContext>&& render_context,
                             std::shared_ptr<CommandContext>&& present_context,
                             uint32_t back_buffer_count);
        RenderGraph::RenderGraphBuilder createRenderGraphBuilder(std::string graph_name);
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