#include <render_engine/ParallelRenderEngine.h>

#include <render_engine/rendergraph/TaskflowBuilder.h>

namespace RenderEngine
{
    namespace
    {
        constexpr uint32_t kMaxNumOfResources = 10;
    }
    using namespace RenderGraph;

    ParallelRenderEngine::ParallelRenderEngine(Device& device,
                                               std::shared_ptr<SingleShotCommandContext>&& transfer_context,
                                               std::shared_ptr<CommandContext>&& render_context,
                                               std::shared_ptr<CommandContext>&& present_context,
                                               uint32_t back_buffer_count)
        : _device(device)
        , _render_context(std::move(render_context))
        , _present_context(std::move(present_context))
        , _transfer_context(std::move(transfer_context))
        , _gpu_resource_manager(device.getPhysicalDevice(), device.getLogicalDevice(), back_buffer_count, kMaxNumOfResources)
    {}

    RenderGraphBuilder ParallelRenderEngine::createRenderGraphBuilder(std::string graph_name)
    {
        return RenderGraphBuilder(std::move(graph_name), _render_context, _present_context, _transfer_context);
    }
    void ParallelRenderEngine::setRenderGraph(std::unique_ptr<RenderGraph::Graph> render_graph)
    {
        if (_skeleton != nullptr)
        {
            throw std::runtime_error("Renderer has already a graph");
        }
        TaskflowBuilder task_flow_builder;

        _rendering_processes.reserve(_gpu_resource_manager->getBackBufferSize());
        _skeleton = std::move(render_graph);

        for (uint32_t i = 0; i < _gpu_resource_manager->getBackBufferSize(); ++i)
        {
            auto rendering_process = std::make_unique<RenderingProcess>();
            rendering_process->task_flow = task_flow_builder.createTaskflow(*_skeleton,
                                                                            rendering_process->execution_contexts,
                                                                            _device.getLogicalDevice(),
                                                                            _gpu_resource_manager->getBackBufferSize());
            _rendering_processes.push_back(rendering_process);
        }
    }

    void ParallelRenderEngine::render()
    {
        if (_skeleton == nullptr)
        {
            return;
        }
        auto& current_process = _rendering_processes[_render_call_count % _rendering_processes.size()];

        if (current_process->calling_token.valid())
        {
            current_process->calling_token.wait();
        }
        current_process->execution_contexts.reset();
        current_process->calling_token = current_process->executor.run(current_process->task_flow);
        _render_call_count++;
    }
}