#pragma once

#include <render_engine/rendergraph/Graph.h>

namespace RenderEngine::RenderGraph
{
    class RenderNode;
    class TransferNode;
    class ComputeNode;
    class PresentNode;
    class CpuNode;
    class Link;

    class GraphVisitor
    {
    public:
        explicit GraphVisitor(Graph& graph)
            : _graph(graph)
        {}

        void run()
        {
            _graph.accept(*this);
        }
        virtual ~GraphVisitor() = default;

        virtual void visit(RenderNode* node) = 0;
        virtual void visit(TransferNode* node) = 0;
        virtual void visit(ComputeNode* node) = 0;
        virtual void visit(PresentNode* node) = 0;
        virtual void visit(CpuNode* node) = 0;
        virtual void visit(Link* edge) = 0;

        Graph& getGraph() { return _graph; }
        const Graph& getGraph() const { return _graph; }
    private:
        Graph& _graph;
    };
}