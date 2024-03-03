#include <render_engine/ParallelRenderEngine.h>

#include <render_engine/rendergraph/TaskflowBuilder.h>

namespace RenderEngine
{
    using namespace RenderGraph;
    void ParallelRenderEngine::RenderGraphBuilder::addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer)
    {
        _graph->addNode(std::make_unique<RenderNode>(std::move(name), _render_engine._render_context, std::move(renderer)));
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
    void ParallelRenderEngine::RenderGraphBuilder::addCpuSyncLink(std::string from, std::string to, SyncObject&& sync_object)
    {
        _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuSync, std::move(sync_object)));
    }
    void ParallelRenderEngine::RenderGraphBuilder::addCpuAsyncLink(std::string from, std::string to, SyncObject&& sync_object)
    {
        _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuAsync, std::move(sync_object)));
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

        for (uint32_t i = 0; i < _gpu_resource_manager->getBackBufferSize(); ++i)
        {
            _rendering_processes.push_back(RenderingProcess{});
        }
        _skeleton = std::move(render_graph);

        for (auto& rendering_process : _rendering_processes)
        {
            rendering_process.task_flow = task_flow_builder.createTaskflow(*_skeleton, rendering_process.execution_contexts);
        }
    }

    void ParallelRenderEngine::render()
    {
        if (_skeleton == nullptr)
        {
            return;
        }
        auto& current_process = _rendering_processes[_render_call_count % _rendering_processes.size()];

        if (current_process.calling_token.valid())
        {
            current_process.calling_token.wait();
        }
        current_process.execution_contexts.reset();
        current_process.calling_token = current_process.executor.run(current_process.task_flow);
        _render_call_count++;
    }
}