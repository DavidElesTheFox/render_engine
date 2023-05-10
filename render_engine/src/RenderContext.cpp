#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

#include <stdexcept>
#include <string>
#include <optional>

#include <SDL.h>
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
		SDL_Init(SDL_INIT_VIDEO);

		if (isVulkanInitialized() == false)
		{
			initVulkan();
		}
	}
	void RenderContext::initVulkan()
	{
		auto appInfo = createAppInfo();
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t extensionCount = 0;
		const char** extensions = nullptr;

		createInfo.enabledExtensionCount = extensionCount;
		createInfo.ppEnabledExtensionNames = extensions;

		createInfo.enabledLayerCount = 0;


		if (VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance); result != VK_SUCCESS)
		{
			throw std::runtime_error("Cannot initialize vulkan instance: " + std::to_string(result));
		}

		auto physical_devices = findPhysicalDevices(_instance);
		if (physical_devices.empty())
		{
			throw std::runtime_error("Cannot find suitable physical devices ");
		}
		for (auto physical_device : physical_devices)
		{
			uint32_t queue_family_index = findQueueFamilies(physical_device).value();
			auto engine = std::make_unique<RenderEngine>(physical_device, queue_family_index);
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
		SDL_Quit();

	}
}