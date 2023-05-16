#include <render_engine/AbstractRenderer.h>

#include <render_engine/SwapChain.h>
#include <cassert>
namespace RenderEngine
{
	AbstractRenderer::ReinitializationCommand::ReinitializationCommand(AbstractRenderer& renderer)
		:_renderer(renderer)
	{
		_renderer.beforeReinit();
	}
	void AbstractRenderer::ReinitializationCommand::finish(const SwapChain& swap_chain)
	{
		_renderer.finalizeReinit(swap_chain);

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