#include <WindowSetup.h>

#include <render_engine/RenderContext.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

SingleWindowSetup::SingleWindowSetup(std::vector<uint32_t> renderer_ids)
{
    using namespace RenderEngine;

    renderer_ids.push_back(UIRenderer::kRendererId);
    _window = RenderContext::context().getDevice(0).createWindow("Demo window", 2);
    _window->registerRenderers(renderer_ids);
}

OffScreenWindowSetup::OffScreenWindowSetup(std::vector<uint32_t> renderer_ids)
{
    using namespace RenderEngine;
    {
        auto graphics_window = RenderContext::context().getDevice(0).createOffScreenWindow("Demo window (render)", 2);
        auto ui_window = RenderContext::context().getDevice(0).createWindow("Demo window", 2);
        _window_tunnel = std::make_unique<WindowTunnel>(std::move(graphics_window), std::move(ui_window));
    }
    _window_tunnel->getOriginWindow().registerRenderers(renderer_ids);
    _window_tunnel->getDestinationWindow().registerRenderers({ ImageStreamRenderer::kRendererId, UIRenderer::kRendererId });
}