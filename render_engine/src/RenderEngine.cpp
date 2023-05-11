#include <render_engine/RenderEngine.h>

#include <render_engine/RenderContext.h>

#include <stdexcept>
#include <optional>
#include <vector>
#include <set>
namespace
{

	VkDevice createLogicalDevice(size_t queue_count, VkPhysicalDevice physical_device, uint32_t queue_family_index_graphics, uint32_t queue_family_index_presentation) {

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { queue_family_index_graphics, queue_family_index_presentation };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queueFamily;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queuePriority;
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = queue_create_infos.size();

		create_info.pEnabledFeatures = &device_features;

		create_info.enabledExtensionCount = 0;


		create_info.enabledLayerCount = 0;
		VkDevice device;
		if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
		return device;

	}
}

namespace RenderEngine
{
	RenderEngine::RenderEngine(VkInstance instance, VkPhysicalDevice physical_device, uint32_t queue_family_index_graphics, uint32_t queue_family_index_presentation)
		: _instance(instance)
		, _physical_device(physical_device)
		, _logical_device(createLogicalDevice(RenderContext::kSupportedWindowCount, physical_device, queue_family_index_graphics, queue_family_index_presentation))
		, _queue_family_present(queue_family_index_presentation)
		, _queue_family_graphics(queue_family_index_graphics)
	{

	}

	RenderEngine::~RenderEngine()
	{
		vkDestroyDevice(_logical_device, nullptr);
	}

	std::unique_ptr<Window> RenderEngine::createWindow(std::string_view name) const
	{
		VkQueue render_queue;
		vkGetDeviceQueue(_logical_device, _queue_family_graphics, _next_queue_index, &render_queue);

		return std::make_unique<Window>(render_queue, name);
	}
}