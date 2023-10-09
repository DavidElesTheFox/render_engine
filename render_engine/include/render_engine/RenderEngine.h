#pragma once

#include <render_engine/resources/Buffer.h>

#include <volk.h>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace RenderEngine
{
	class Window;


	class RenderEngine
	{
	public:
		static constexpr auto kSupportedWindowCount = 1;

		explicit RenderEngine(VkInstance instance,
			VkPhysicalDevice physical_device,
			uint32_t queue_family_index_graphics,
			uint32_t queue_family_index_presentation,
			const std::vector<const char*>& device_extensions,
			const std::vector<const char*>& validation_layers);
		RenderEngine(const RenderEngine&) = delete;
		RenderEngine(RenderEngine&&) = delete;

		RenderEngine& operator=(const RenderEngine&) = delete;
		RenderEngine& operator=(RenderEngine&&) = delete;

		~RenderEngine();
		std::unique_ptr<Window> createWindow(std::string_view name, size_t back_buffer_size);


		VkDevice getLogicalDevice() { return _logical_device; }
		VkPhysicalDevice getPhysicalDevice() { return _physical_device; }
		VkInstance& getVulkanInstance() { return _instance; }
	private:
		VkInstance _instance;
		VkPhysicalDevice _physical_device;
		VkDevice _logical_device;
		uint32_t _queue_family_present = 0;
		uint32_t _queue_family_graphics = 0;
		uint32_t _next_queue_index = 0;
	};
}
