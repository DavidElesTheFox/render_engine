#include <render_engine/rendergraph/RenderGraphBuilder.h>

#include <render_engine/rendergraph/Graph.h>
#include <render_engine/window/SwapChain.h>

namespace RenderEngine::RenderGraph
{
    RenderGraphBuilder::RenderGraphBuilder(std::string name,
                                           std::weak_ptr<CommandContext> render_context,
                                           std::weak_ptr<CommandContext> present_context,
                                           std::weak_ptr<SingleShotCommandContext> transfer_context)
        : _graph(std::make_unique<RenderGraph::Graph>(std::move(name)))
        , _render_context(render_context)
        , _present_context(present_context)
        , _transfer_context(transfer_context)
    {

    }

    void RenderGraphBuilder::addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer)
    {
        // TODO add debug UI
        constexpr bool enable_tracking = false;
        _graph->addNode(std::make_unique<RenderNode>(std::move(name), _render_context.lock(), std::move(renderer), enable_tracking));
    }
    void RenderGraphBuilder::addTransferNode(std::string name)
    {
        TransferEngine transfer_engine(_transfer_context.lock());
        auto node = std::make_unique<TransferNode>(std::move(name), std::move(transfer_engine));
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addComputeNode(std::string name, std::unique_ptr<RenderGraph::ComputeNode::IComputeTask> task)
    {
        auto node = std::make_unique<ComputeNode>(std::move(name), std::move(task));
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addCpuNode(std::string name, std::unique_ptr<RenderGraph::CpuNode::ICpuTask> task)
    {
        auto node = std::make_unique<CpuNode>(std::move(name), std::move(task));
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addPresentNode(std::string name, SwapChain& swap_chain)
    {
        auto node = std::make_unique<PresentNode>(std::move(name), swap_chain, _present_context.lock());
        _graph->addNode(std::move(node));
    }
    void RenderGraphBuilder::addEmptyNode(std::string name)
    {
        auto node = std::make_unique<NullNode>(std::move(name));
        _graph->addNode(std::move(node));
    }
    RenderGraphBuilder::GeneralGpuLinkBuilder RenderGraphBuilder::addCpuSyncLink(const std::string& from, const std::string& to)
    {
        auto& link = _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuSync));
        return GpuLinkBuilder(link);
    }
    RenderGraphBuilder::GeneralGpuLinkBuilder RenderGraphBuilder::addCpuAsyncLink(const std::string& from, const std::string& to)
    {
        auto& link = _graph->addEdge(std::make_unique<Link>(_graph->findNode(from), _graph->findNode(to), LinkType::CpuAsync));
        return GpuLinkBuilder(link);
    }


}