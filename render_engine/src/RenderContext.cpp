#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

#include <stdexcept>
#include <string>
#include <optional>
#include <set>
#include <vulkan/vk_enum_string_helper.h>

#include <GLFW/glfw3.h>
namespace
{
	VkApplicationInfo createAppInfo() {
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "TestApplication";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Boo";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		return appInfo;
	}

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_index;
		std::optional<uint32_t> presentation_index;

		bool hasAll() const { return graphics_index != std::nullopt && presentation_index != std::nullopt; }
	};

	QueueFamilyIndices findQueueFamilies(VkInstance instance, VkPhysicalDevice device) {

		QueueFamilyIndices result;
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		for (uint32_t i = 0; i < queue_families.size(); ++i) {
			const auto& queue_family = queue_families[i];

			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				result.graphics_index = i;
			}

			if (glfwGetPhysicalDevicePresentationSupport(instance, device, i))
			{
				result.presentation_index = i;
			}
			if (result.hasAll())
			{
				break;
			}
		}

		return result;
	}
	bool checkDeviceExtensionSupport(VkPhysicalDevice physical_device, const std::vector<const char*>& extensions) {
		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> missing_extensions(extensions.begin(), extensions.end());

		for (const auto& extension : available_extensions) {
			missing_extensions.erase(extension.extensionName);
		}

		return missing_extensions.empty();
	}
	bool isDeviceSuitable(VkInstance instance, VkPhysicalDevice device, const std::vector<const char*>& extensions) {
		auto indecies = findQueueFamilies(instance, device);

		return indecies.hasAll() && checkDeviceExtensionSupport(device, extensions);
	}

	std::vector<VkPhysicalDevice> findPhysicalDevices(VkInstance instance, std::vector<const char*> extensions) {
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		if (device_count == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

		devices.erase(std::remove_if(devices.begin(), devices.end(),
			[&](const auto& device) {return isDeviceSuitable(instance, device, extensions) == false; }),
			devices.end());
		return devices;
	}

}
namespace RenderEngine
{
	RenderContext& RenderContext::context()
	{
		static RenderContext context;
		if (context.isVulkanInitialized() == false)
		{
			context.initVulkan();
		}
		return context;
	}


	RenderContext::RenderContext()
	{
		init();
	}
	void RenderContext::init()
	{
		glfwInit();
		if (isVulkanInitialized() == false)
		{
			initVulkan();
			createEngines();
		}
	}
	void RenderContext::initVulkan()
	{
		auto app_info = createAppInfo();
		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		std::vector<const char*> instance_extensions = {  };
		{
			uint32_t glfw_extension_count = 0;
			const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
			for (uint32_t i = 0; i < glfw_extension_count; ++i)
			{
				instance_extensions.push_back(glfw_extensions[i]);
			}
		}

		create_info.enabledExtensionCount = instance_extensions.size();
		create_info.ppEnabledExtensionNames = instance_extensions.data();

		create_info.enabledLayerCount = 0;


		if (VkResult result = vkCreateInstance(&create_info, nullptr, &_instance); result != VK_SUCCESS)
		{
			throw std::runtime_error(std::string{ "Cannot initialize vulkan instance: " } + string_VkResult(result));
		}
	}

	void RenderContext::createEngines()
	{
		std::vector<const char*> device_extensions{ Window::kDeviceExtensions.begin(), Window::kDeviceExtensions.end() };

		auto physical_devices = findPhysicalDevices(_instance, device_extensions);
		if (physical_devices.empty())
		{
			throw std::runtime_error("Cannot find suitable physical devices ");
		}
		for (auto physical_device : physical_devices)
		{
			auto indices = findQueueFamilies(_instance, physical_device);
			auto engine = std::make_unique<RenderEngine>(_instance, physical_device, *indices.graphics_index, *indices.presentation_index, device_extensions);
			_engines.push_back(std::move(engine));
		}
	}

	void RenderContext::reset()
	{
		if (isVulkanInitialized())
		{
			_engines.clear();
			vkDestroyInstance(_instance, nullptr);
			_instance = nullptr;
		}
		glfwTerminate();
	}
}