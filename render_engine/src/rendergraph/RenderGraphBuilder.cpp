#include <render_engine/rendergraph/RenderGraphBuilder.h>

#include <render_engine/rendergraph/Graph.h>
#include <render_engine/window/SwapChain.h>

namespace RenderEngine::RenderGraph
{
    RenderGraphBuilder::RenderGraphBuilder(std::string name,
                                           CommandBufferContext render_context,
                                           CommandBufferContext present_context,
                                           TransferEngine transfer_engine)
        : _graph(std::make_unique<RenderGraph::Graph>(std::move(name)))
        , _render_context(std::move(render_context))
        , _present_context(std::move(present_context))
        , _transfer_engine(transfer_engine)
    {

    }

    void RenderGraphBuilder::addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer)
    {
        _graph->addNode(std::make_unique<RenderNode>(std::move(name), _render_context.getFactory(), std::move(renderer)));
    }
    void RenderGraphBuilder::addTransferNode(std::string name)
    {
        auto node = std::make_unique<TransferNode>(std::move(name), _transfer_engine);
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addDeviceSynchronizeNode(std::string name, Device& device)
    {
        auto node = std::make_unique<DeviceSynchronizeNode>(std::move(name), device);
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addComputeNode(std::string name, std::unique_ptr<RenderGraph::ComputeNode::IComputeTask> task)
    {
        auto node = std::make_unique<ComputeNode>(std::move(name), std::move(task));
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addCpuNode(std::string name, std::unique_ptr<RenderGraph::CpuNode::ICpuTask> task)
    {
        auto node = std::make_unique<CpuNode>(std::move(name), _render_context.getFactory(), std::move(task));
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addPresentNode(std::string name, SwapChain& swap_chain)
    {
        auto node = std::make_unique<PresentNode>(std::move(name), swap_chain, _present_context.getFactory());
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addEmptyNode(std::string name)
    {
        auto node = std::make_unique<NullNode>(std::move(name));
        _graph->addNode(std::move(node));
    }
    RenderGraphBuilder::GeneralGpuLinkBuilder RenderGraphBuilder::addCpuSyncLink(const std::string& from, const std::string& to)
    {
        _graph->applyChanges();
        auto& link = _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuSync));
        return GeneralGpuLinkBuilder(link, *_graph);
    }
    RenderGraphBuilder::GeneralGpuLinkBuilder RenderGraphBuilder::addCpuAsyncLink(const std::string& from, const std::string& to)
    {
        _graph->applyChanges();
        auto& link = _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuAsync));
        return GeneralGpuLinkBuilder(link, *_graph);
    }


}