#pragma once

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>


namespace RenderEngine
{
	class RenderEngine;
	class RenderContext
	{

	public:
		static RenderContext& context();

		RenderEngine& getEngine(size_t index) const
		{
			return *_engines[index];
		}
		void reset();
	private:
		RenderContext();
		~RenderContext()
		{
			reset();
		}
		void init();
		void initVulkan(const std::vector<const char*>& validation_layers);
		void createEngines(const std::vector<const char*>& validation_layers);
		bool isVulkanInitialized() const { return _instance != nullptr; }
		VkInstance _instance;
		std::vector<std::unique_ptr<RenderEngine>> _engines;
		VkDebugUtilsMessengerEXT _debug_messenger;
	};
}