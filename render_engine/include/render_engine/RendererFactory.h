#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/resources/RenderTarget.h>

namespace RenderEngine
{
    class Window;
    class SwapChain;
    class RendererFeactory
    {
    public:
        RendererFeactory() = default;

        void registerRenderer(uint32_t id, std::function<std::unique_ptr<AbstractRenderer>(Window&, const RenderTarget&, uint32_t, AbstractRenderer*, bool)> factory_method);
        std::vector<std::unique_ptr<AbstractRenderer>> generateRenderers(std::vector<uint32_t> renderer_ids,
                                                                         Window& window,
                                                                         const RenderTarget& render_target,
                                                                         uint32_t back_buffer_count);
    private:
        std::vector<uint32_t> _active_renderers;
        std::unordered_map<uint32_t, std::function<std::unique_ptr<AbstractRenderer>(Window&, const RenderTarget&, uint32_t, AbstractRenderer*, bool)>> _factory_methods;
    };
}