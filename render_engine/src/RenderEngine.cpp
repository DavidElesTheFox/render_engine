#include <render_engine/RenderEngine.h>

#include <render_engine/RenderContext.h>

#include <stdexcept>
#include <optional>
#include <vector>
namespace
{
	std::optional<uint32_t> findQueueFamilies(VkPhysicalDevice device) {

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
			const auto& queueFamily = queueFamilies[i];
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				return i;
			}
		}

		return std::nullopt;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {
		auto index = findQueueFamilies(device);

		return index != std::nullopt;
	}

	std::vector<VkPhysicalDevice> findPhysicalDevices(VkInstance instance) {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		devices.erase(std::remove_if(devices.begin(), devices.end(),
			[](const auto& device) {return isDeviceSuitable(device) == false; }),
			devices.end());
	}

	VkDevice createLogicalDevice(size_t queue_count, VkPhysicalDevice physicalDevice, uint32_t queue_family_index) {

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queue_family_index;
		queueCreateInfo.queueCount = queue_count;

		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = 0;


		createInfo.enabledLayerCount = 0;
		VkDevice device;
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
		return device;

	}
}

namespace RenderEngine
{
	RenderEngine::RenderEngine(VkPhysicalDevice physical_device, uint32_t queue_family_index)
		: _physical_device(physical_device)
		, _logical_device(createLogicalDevice(RenderContext::kSupportedWindowCount, physical_device, queue_family_index))
		, _queue_family_index(queue_family_index)
	{

	}


	RenderEngine::~RenderEngine()
	{
		vkDestroyDevice(_logical_device, nullptr);
	}

	std::unique_ptr<Window> RenderEngine::createWindow(std::string_view name) const
	{
		VkQueue render_queue;
		vkGetDeviceQueue(_logical_device, _queue_family_index, _next_queue_index, &render_queue);

		return std::make_unique<Window>(render_queue, name);
	}
}