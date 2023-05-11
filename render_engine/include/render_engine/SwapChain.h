#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace RenderEngine
{
	class SwapChain
	{
	public:
		struct Details
		{
			VkSwapchainKHR swap_chain;
			std::vector<VkImage> images;
			std::vector<VkImageView> image_views;
			VkFormat image_format;
			VkExtent2D extent;
			VkSurfaceKHR surface;
		};
		SwapChain(GLFWwindow* window,
			VkInstance instance,
			VkPhysicalDevice physical_device,
			VkDevice logical_device,
			VkSurfaceKHR&& surface,
			uint32_t graphics_family_index,
			uint32_t present_family_index);

		~SwapChain();
		const Details& getDetails() const { return _details; }


	private:
		Details _details;
		VkInstance _instance;
		VkDevice _logical_device;
	};
}