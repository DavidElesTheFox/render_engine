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
		static constexpr auto kSupportedWindowCount = 8;
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
		void initVulkan();
		bool isVulkanInitialized() const { return _instance != nullptr; }
		VkInstance _instance;
		std::vector<std::unique_ptr<RenderEngine>> _engines;
	};
}