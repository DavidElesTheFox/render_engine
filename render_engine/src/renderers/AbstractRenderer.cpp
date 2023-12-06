#include <render_engine/renderers/AbstractRenderer.h>

#include <render_engine/window/SwapChain.h>

#include <cassert>
namespace RenderEngine
{
    AbstractRenderer::ReinitializationCommand::ReinitializationCommand(AbstractRenderer& renderer)
        :_renderer(renderer)
    {
        _renderer.beforeReinit();
    }
    void AbstractRenderer::ReinitializationCommand::finish(const RenderTarget& render_target)
    {
        _renderer.finalizeReinit(render_target);
        _finished = true;
    }
    AbstractRenderer::ReinitializationCommand::~ReinitializationCommand()
    {
        assert(_finished);
    }
    AbstractRenderer::ReinitializationCommand AbstractRenderer::reinit()
    {
        return ReinitializationCommand(*this);
    }
}