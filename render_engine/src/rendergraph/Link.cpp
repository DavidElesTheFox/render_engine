#include <render_engine/rendergraph/Link.h>

#include <render_engine/rendergraph/GraphVisitor.h>

namespace RenderEngine::RenderGraph
{
    void Link::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }
}