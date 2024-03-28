#include <render_engine/ParallelRenderEngine.h>

#include <render_engine/rendergraph/TaskflowBuilder.h>

namespace RenderEngine
{
    using namespace RenderGraph;
    void ParallelRenderEngine::RenderGraphBuilder::addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer)
    {
        // TODO add debug UI
        constexpr bool enable_tracking = false;
        _graph->addNode(std::make_unique<RenderNode>(std::move(name), _render_engine._render_context, std::move(renderer), enable_tracking));
    }
    void ParallelRenderEngine::RenderGraphBuilder::addTransferNode(std::string name)
    {
        TransferEngine transfer_engine(_render_engine._transfer_context);
        auto node = std::make_unique<TransferNode>(std::move(name), std::move(transfer_engine));
        _graph->addNode(std::move(node));
    }
    void ParallelRenderEngine::RenderGraphBuilder::addComputeNode(std::string name, std::unique_ptr<RenderGraph::ComputeNode::IComputeTask> task)
    {
        auto node = std::make_unique<ComputeNode>(std::move(name), std::move(task));
        _graph->addNode(std::move(node));
    }
    void ParallelRenderEngine::RenderGraphBuilder::addCpuNode(std::string name, std::unique_ptr<RenderGraph::CpuNode::ICpuTask> task)
    {
        auto node = std::make_unique<CpuNode>(std::move(name), std::move(task));
        _graph->addNode(std::move(node));
    }
    void ParallelRenderEngine::RenderGraphBuilder::addPresentNode(std::string name, SwapChain& swap_chain)
    {
        auto node = std::make_unique<PresentNode>(std::move(name), swap_chain, _render_engine._present_context);
        _graph->addNode(std::move(node));
    }
    ParallelRenderEngine::RenderGraphBuilder::GpuLinkBuilder ParallelRenderEngine::RenderGraphBuilder::addCpuSyncLink(const std::string& from, const std::string& to)
    {
        auto& link = _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuSync));
        return GpuLinkBuilder(link);
    }
    ParallelRenderEngine::RenderGraphBuilder::GpuLinkBuilder ParallelRenderEngine::RenderGraphBuilder::addCpuAsyncLink(const std::string& from, const std::string& to)
    {
        auto& link = _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuAsync));
        return GpuLinkBuilder(link);
    }

    ParallelRenderEngine::RenderGraphBuilder ParallelRenderEngine::createRenderGraphBuilder(std::string graph_name)
    {
        return RenderGraphBuilder(std::move(graph_name), *this);
    }
    void ParallelRenderEngine::setRenderGraph(std::unique_ptr<RenderGraph::Graph> render_graph)
    {
        if (_skeleton != nullptr)
        {
            throw std::runtime_error("Renderer has already a graph");
        }
        TaskflowBuilder task_flow_builder;

        _rendering_processes.reserve(_gpu_resource_manager->getBackBufferSize());
        for (uint32_t i = 0; i < _gpu_resource_manager->getBackBufferSize(); ++i)
        {
            _rendering_processes.push_back(std::make_unique<RenderingProcess>());
        }
        _skeleton = std::move(render_graph);

        for (auto& rendering_process : _rendering_processes)
        {
            rendering_process->task_flow = task_flow_builder.createTaskflow(*_skeleton,
                                                                            rendering_process->execution_contexts,
                                                                            _device.getLogicalDevice(),
                                                                            _gpu_resource_manager->getBackBufferSize());
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