#pragma once

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include <render_engine/RendererFactory.h>

namespace RenderEngine
{
	class RenderEngine;
	class RenderContext
	{

	public:
		static RenderContext& context();
		static void initialize(const std::vector<const char*>& validation_layers, std::unique_ptr<RendererFeactory> renderer_factory);

		RenderEngine& getEngine(size_t index) const
		{
			return *_engines[index];
		}
		void reset();
		void* getRenderdocApi();
		RendererFeactory& getRendererFactory()
		{
			return *_renderer_factory;
		}
	private:
		static RenderContext& context_impl();

		RenderContext();
		~RenderContext()
		{
			reset();
		}
		void init(const std::vector<const char*>& validation_layers, std::unique_ptr<RendererFeactory> renderer_factory);
		void initVulkan(const std::vector<const char*>& validation_layers);
		void createEngines(const std::vector<const char*>& validation_layers);
		bool isVulkanInitialized() const { return _instance != nullptr; }
		VkInstance _instance;
		std::vector<std::unique_ptr<RenderEngine>> _engines;
		VkDebugUtilsMessengerEXT _debug_messenger;
		void* _renderdoc_api{ nullptr };
		std::unique_ptr<RendererFeactory> _renderer_factory;
		bool _initialized{ false };
	};
}