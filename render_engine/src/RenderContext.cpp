#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <optional>
#include <set>
#include <vector>
#include <iostream>

#include <vulkan/vk_enum_string_helper.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#ifdef ENABLE_RENDERDOC
#include <data_config.h>
#include <renderdoc_app.h>
#endif
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
	constexpr bool g_enable_validation_layers = true;

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
	bool checkValidationLayerSupport(const std::vector<const char*>& validation_layers) {
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (std::string_view layer_name : validation_layers) {
			bool layerFound = false;

			for (const auto& properties : available_layers) {
				if (layer_name == properties.layerName) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void*) {
		if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			return VK_FALSE;
		}
		std::cerr << "validation layer: " << data->pMessage << std::endl;

		return VK_FALSE;
	}

}
namespace RenderEngine
{
	RenderContext& RenderContext::context()
	{
		RenderContext& result = context_impl();
		assert(result._initialized);
		return result;
	}
	void RenderContext::initialize(const std::vector<const char*>& validation_layers, std::unique_ptr<RendererFeactory> renderer_factory)
	{
		auto& context = context_impl();
		context.init(validation_layers, std::move(renderer_factory));
	}
	RenderContext& RenderContext::context_impl()
	{
		static RenderContext context;
		return context;
	}


	RenderContext::RenderContext()
	{
	}
	void RenderContext::init(const std::vector<const char*>& validation_layers, std::unique_ptr<RendererFeactory> renderer_factory)
	{
#ifdef ENABLE_RENDERDOC

		if (HMODULE mod = GetModuleHandleA(RENDERDOC_DLL))
		{
			pRENDERDOC_GetAPI RENDERDOC_GetAPI =
				(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
			int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&_renderdoc_api);
			if (ret != 1)
			{
				throw std::runtime_error("Cannot open renderdoc dll");
			}
		}
#endif
		_renderer_factory = std::move(renderer_factory);
		const bool enable_validation_layer = validation_layers.empty() == false;

		glfwInit();
		if (isVulkanInitialized() == false)
		{
			initVulkan(validation_layers);
			createEngines(validation_layers);
		}
	}
	void RenderContext::initVulkan(const std::vector<const char*>& validation_layers)
	{
		volkInitialize();
		if (g_enable_validation_layers && checkValidationLayerSupport(validation_layers) == false)
		{
			throw std::runtime_error("Validation layers requested but not available");
		}
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
			if (g_enable_validation_layers) {
				instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

		}

		create_info.enabledExtensionCount = instance_extensions.size();
		create_info.ppEnabledExtensionNames = instance_extensions.data();
		VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};

		if (g_enable_validation_layers)
		{
			debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debug_create_info.pfnUserCallback = debugCallback;

			create_info.enabledLayerCount = validation_layers.size();
			create_info.ppEnabledLayerNames = validation_layers.data();
			create_info.pNext = &debug_create_info;
		}
		else
		{
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}

		if (VkResult result = vkCreateInstance(&create_info, nullptr, &_instance); result != VK_SUCCESS)
		{
			throw std::runtime_error(std::string{ "Cannot initialize vulkan instance: " } + string_VkResult(result));
		}
		volkLoadInstance(_instance);
	}

	void RenderContext::createEngines(const std::vector<const char*>& validation_layers)
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
			auto engine = std::make_unique<RenderEngine>(_instance,
				physical_device,
				*indices.graphics_index,
				*indices.presentation_index,
				device_extensions,
				validation_layers);
			_engines.push_back(std::move(engine));
#ifdef ENABLED_RENDERDOC
			break; // Renderdoc supports only one logical device
#endif
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
	void* RenderContext::getRenderdocApi()
	{
#ifdef ENABLE_RENDERDOC
		return _renderdoc_api;
#else
		throw std::runtime_error("Cannot enable renderdoc, feature is disabled in this build");
#endif
	}
	}