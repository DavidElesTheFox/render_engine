#include <render_engine/RendererFactory.h>

#include <stdexcept>
#include <string>
namespace RenderEngine
{
	void RendererFeactory::registerRenderer(uint32_t id, std::function<std::unique_ptr<AbstractRenderer>(Window&, SwapChain&, uint32_t, bool)> factory_method)
	{
		bool inserted = _factory_methods.insert({ id, factory_method }).second;
		if (inserted == false)
		{
			throw std::runtime_error("Renderer with id " + std::to_string(id) + " is already registered");
		}
	}
	std::vector<std::unique_ptr<AbstractRenderer>> RendererFeactory::generateRenderers(std::vector<uint32_t> renderer_ids,
		Window& window,
		SwapChain& swap_chain,
		uint32_t back_buffer_count)
	{
		std::vector<std::unique_ptr<AbstractRenderer>> result;
		for (uint32_t renderer_id : renderer_ids)
		{
			auto it = _factory_methods.find(renderer_id);
			if (it == _factory_methods.end())
			{
				throw std::runtime_error("Renderer id is not registered: " + std::to_string(renderer_id));
			}
			result.emplace_back(it->second(window, swap_chain, back_buffer_count, renderer_id == renderer_ids.back()));
		}
		return result;
	}
}