#include <render_engine/RenderEngine.h>
#include <volk.h>

#include <render_engine/RenderContext.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/window/Window.h>

#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_win32.h>
#include <stdexcept>
#include <optional>
#include <vector>
#include <set>
namespace
{

	VkDevice createLogicalDevice(size_t queue_count,
		VkPhysicalDevice physical_device,
		uint32_t queue_family_index_graphics,
		uint32_t queue_family_index_presentation,
		const std::vector<const char*>& device_extensions,
		const std::vector<const char*>& validation_layers) {

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { queue_family_index_graphics, queue_family_index_presentation };

		std::vector<float> queuePriority(queue_count, 1.0f);
		for (uint32_t queueFamily : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queueFamily;
			queue_create_info.queueCount = queue_count;
			queue_create_info.pQueuePriorities = queuePriority.data();
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = queue_create_infos.size();

		create_info.pEnabledFeatures = &device_features;

		create_info.enabledExtensionCount = device_extensions.size();
		create_info.ppEnabledExtensionNames = device_extensions.data();

		if (validation_layers.empty())
		{
			create_info.enabledLayerCount = 0;
		}
		else
		{
			create_info.enabledLayerCount = validation_layers.size();
			create_info.ppEnabledLayerNames = validation_layers.data();
		}
		VkDevice device;
		if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
		return device;

	}
}

namespace RenderEngine
{
	RenderEngine::RenderEngine(VkInstance instance,
		VkPhysicalDevice physical_device,
		uint32_t queue_family_index_graphics,
		uint32_t queue_family_index_presentation,
		const std::vector<const char*>& device_extensions,
		const std::vector<const char*>& validation_layers)
		: _instance(instance)
		, _physical_device(physical_device)
		, _logical_device(createLogicalDevice(kSupportedWindowCount, physical_device, queue_family_index_graphics, queue_family_index_presentation, device_extensions, validation_layers))
		, _queue_family_present(queue_family_index_presentation)
		, _queue_family_graphics(queue_family_index_graphics)
	{

	}

	RenderEngine::~RenderEngine()
	{
		vkDestroyDevice(_logical_device, nullptr);
	}

	std::unique_ptr<Window> RenderEngine::createWindow(std::string_view name, size_t back_buffer_size)
	{
		VkQueue render_queue;
		VkQueue present_queue;
		vkGetDeviceQueue(_logical_device, _queue_family_graphics, _next_queue_index, &render_queue);
		vkGetDeviceQueue(_logical_device, _queue_family_present, _next_queue_index, &present_queue);

		constexpr auto width = 1024;
		constexpr auto height = 764;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow* window = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);

		VkWin32SurfaceCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		create_info.hwnd = glfwGetWin32Window(window);
		create_info.hinstance = GetModuleHandle(nullptr);
		VkSurfaceKHR surface;

		if (vkCreateWin32SurfaceKHR(_instance, &create_info, nullptr, &surface) != VK_SUCCESS) {
			glfwDestroyWindow(window);
			throw std::runtime_error("Failed to create window surface!");
		}
		try
		{
			std::unique_ptr<SwapChain> swap_chain = std::make_unique<SwapChain>(SwapChain::CreateInfo{ window, _instance, _physical_device, _logical_device, std::move(surface), _queue_family_graphics, _queue_family_present });
			_next_queue_index = (_next_queue_index + 1) % kSupportedWindowCount;
			return std::make_unique<Window>(*this, window, std::move(swap_chain), render_queue, present_queue, _queue_family_graphics, back_buffer_size);
		}
		catch (const std::runtime_error& error)
		{
			glfwDestroyWindow(window);
			throw;
		}
	}

}