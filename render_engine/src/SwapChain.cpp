#include <render_engine/SwapChain.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
namespace
{
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

		if (format_count != 0) {
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
		}

		uint32_t present_mode_count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

		if (present_mode_count != 0) {
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
		}

		return details;
	}
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for (const auto& available_format : available_formats) {
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return available_format;
			}
		}

		return available_formats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
		for (const auto& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return available_present_mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
		if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
			return capabilities.currentExtent;
		}
		else {
			int width = 0, height = 0;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actual_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}

	std::vector<VkImageView> createImageViews(const std::vector<VkImage>& images, VkFormat format, VkDevice logical_device)
	{
		std::vector<VkImageView> image_views;
		image_views.resize(images.size());

		for (size_t i = 0; i < images.size(); i++) {
			VkImageViewCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = images[i];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = format;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(logical_device, &create_info, nullptr, &image_views[i]) != VK_SUCCESS) {
				for (uint32_t j = 0; j < i; ++j)
				{
					vkDestroyImageView(logical_device, image_views[j], nullptr);
				}
				throw std::runtime_error("failed to create image views!");
			}
		}
		return image_views;
	}

	RenderEngine::SwapChain::Details createSwapChain(VkInstance instance,
		GLFWwindow* window,
		VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkSurfaceKHR&& surface,
		uint32_t graphics_family_index,
		uint32_t present_family_index)
	{
		SwapChainSupportDetails swap_chain_support = querySwapChainSupport(physical_device, surface);
		if (swap_chain_support.present_modes.empty())
		{
			throw std::runtime_error("Cannot create swap chain. There is no valid presentation mode on device");
		}
		VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
		VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
		VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities, window);

		uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
		if (0 < swap_chain_support.capabilities.maxImageCount && swap_chain_support.capabilities.maxImageCount < image_count) {
			image_count = swap_chain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = image_count;
		createInfo.imageFormat = surface_format.format;
		createInfo.imageColorSpace = surface_format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		std::array<uint32_t, 2> queueFamilyIndices = { graphics_family_index, present_family_index };

		if (graphics_family_index != present_family_index) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swap_chain_support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = present_mode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		RenderEngine::SwapChain::Details result;

		if (vkCreateSwapchainKHR(logical_device, &createInfo, nullptr, &result.swap_chain) != VK_SUCCESS) {
			vkDestroySurfaceKHR(instance, surface, nullptr);
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logical_device, result.swap_chain, &image_count, nullptr);
		result.images.resize(image_count);
		vkGetSwapchainImagesKHR(logical_device, result.swap_chain, &image_count, result.images.data());

		result.image_format = surface_format.format;
		result.extent = extent;
		result.surface = std::move(surface);
		try
		{
			result.image_views = createImageViews(result.images, result.image_format, logical_device);
		}
		catch (const std::runtime_error&)
		{
			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroySwapchainKHR(logical_device, result.swap_chain, nullptr);
			throw;
		}
		return result;
	}


}

namespace RenderEngine
{
	SwapChain::SwapChain(GLFWwindow* window,
		VkInstance instance,
		VkPhysicalDevice physical_device,
		VkDevice logical_device,
		VkSurfaceKHR&& surface,
		uint32_t graphics_family_index,
		uint32_t present_family_index)
		: _details{ createSwapChain(instance, window, physical_device, logical_device, std::move(surface), graphics_family_index, present_family_index) }
		, _instance(instance)
		, _logical_device(logical_device)
	{
	}

	SwapChain::~SwapChain()
	{
		for (VkImageView& image_view : _details.image_views)
		{
			vkDestroyImageView(_logical_device, image_view, nullptr);
		}
		vkDestroySwapchainKHR(_logical_device, _details.swap_chain, nullptr);
		vkDestroySurfaceKHR(_instance, _details.surface, nullptr);

	}
}