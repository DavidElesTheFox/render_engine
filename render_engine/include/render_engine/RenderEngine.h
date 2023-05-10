#pragma once

#include <render_engine/Window.h>

#include <vulkan/vulkan.h>
#include <memory>

namespace RenderEngine
{
	class RenderEngine
	{
	public:
		explicit RenderEngine(VkPhysicalDevice physical_device, uint32_t queue_family_index);
		RenderEngine(const RenderEngine&) = delete;
		RenderEngine(RenderEngine&&) = delete;

		RenderEngine& operator=(const RenderEngine&) = delete;
		RenderEngine& operator=(RenderEngine&&) = delete;

		~RenderEngine();
		std::unique_ptr<Window> createWindow(std::string_view name) const;

	private:
		VkPhysicalDevice _physical_device;
		VkDevice _logical_device;
		uint32_t _queue_family_index;
		uint32_t _next_queue_index = 0;
	};
}