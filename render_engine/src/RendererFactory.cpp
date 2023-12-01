#include <render_engine/RendererFactory.h>

#include <stdexcept>
#include <string>
namespace RenderEngine
{
    void RendererFeactory::registerRenderer(uint32_t id, std::function<std::unique_ptr<AbstractRenderer>(Window&, const RenderTarget&, uint32_t, AbstractRenderer*, bool)> factory_method)
    {
        bool inserted = _factory_methods.insert({ id, factory_method }).second;
        if (inserted == false)
        {
            throw std::runtime_error("Renderer with id " + std::to_string(id) + " is already registered");
        }
    }
    std::vector<std::unique_ptr<AbstractRenderer>> RendererFeactory::generateRenderers(std::vector<uint32_t> renderer_ids,
                                                                                       Window& window,
                                                                                       const RenderTarget& render_target,
                                                                                       uint32_t back_buffer_count)
    {
        std::vector<std::unique_ptr<AbstractRenderer>> result;
        for (size_t i = 0; i < renderer_ids.size(); ++i)
        {
            uint32_t renderer_id = renderer_ids[i];
            auto it = _factory_methods.find(renderer_id);
            if (it == _factory_methods.end())
            {
                throw std::runtime_error("Renderer id is not registered: " + std::to_string(renderer_id));
            }
            AbstractRenderer* prev_renderer = i == 0 ? nullptr : result[0].get();
            result.emplace_back(it->second(window, render_target, back_buffer_count, prev_renderer, i == (renderer_ids.size() - 1)));
        }
        return result;
    }
}