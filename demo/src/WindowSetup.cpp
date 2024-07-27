#include <WindowSetup.h>

#include <render_engine/RenderContext.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

SingleWindowSetup::SingleWindowSetup(std::vector<uint32_t> renderer_ids, bool use_parallel_engine)
{
    using namespace RenderEngine;

    renderer_ids.push_back(UIRenderer::kRendererId);
    if (use_parallel_engine)
    {
        _window = RenderContext::context().getDevice(0).createParallelWindow("Demo window Parallel Engine",
                                                                             getBackbufferCount(),
                                                                             getParallelFrameCount());
    }
    else
    {
        _window = RenderContext::context().getDevice(0).createWindow("Demo window", this->getBackbufferCount());
        _window->registerRenderers(renderer_ids);
    }
}

OffScreenWindowSetup::OffScreenWindowSetup(std::vector<uint32_t> renderer_ids)
{
    using namespace RenderEngine;
    {
        auto graphics_window = RenderContext::context().getDevice(0).createOffScreenWindow(this->getBackbufferCount());
        auto ui_window = RenderContext::context().getDevice(0).createWindow("Demo window", this->getBackbufferCount());
        _window_tunnel = std::make_unique<WindowTunnel>(std::move(graphics_window), std::move(ui_window));
    }
    _window_tunnel->getOriginWindow().registerRenderers(renderer_ids);
    _window_tunnel->getDestinationWindow().registerRenderers({ ImageStreamRenderer::kRendererId, UIRenderer::kRendererId });
}