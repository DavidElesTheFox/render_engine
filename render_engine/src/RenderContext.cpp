#include <render_engine/RenderContext.h>

#include <volk.h>

#include <render_engine/Device.h>

#include <algorithm>
#include <iostream>
#include <optional>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <backends/imgui_impl_glfw.h>
#include <vulkan/vk_enum_string_helper.h>

#include <GLFW/glfw3.h>

#include <data_config.h>
#include <imgui.h>

#ifdef ENABLE_RENDERDOC
#include <renderdoc_app.h>
#endif

#include <render_engine/window/Window.h>
#include <vulkan/vulkan_win32.h>
namespace
{

    bool checkLayerSupport(const std::vector<std::string>& layers)
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (std::string_view layer_name : layers)
        {
            bool layerFound = false;

            for (const auto& properties : available_layers)
            {
                if (layer_name == properties.layerName)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                 void*)
    {
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

    void RenderContext::clearGarbage()
    {
        std::erase_if(_garbage, [](auto& data) { data.life_count--; return data.life_count == 0; });
    }
    RenderContext& RenderContext::context()
    {
        RenderContext& result = context_impl();
        assert(result._initialized);
        return result;
    }
    void RenderContext::initialize(InitializationInfo&& info)
    {
        auto& context = context_impl();
        context.init(std::move(info));
    }
    RenderContext& RenderContext::context_impl()
    {
        static RenderContext context;
        return context;
    }


    void RenderContext::init(InitializationInfo&& info)
    {
#ifdef ENABLE_RENDERDOC
        if (HMODULE mod = LoadLibraryA(RENDERDOC_DLL); mod == nullptr)
        {
            throw std::runtime_error("Cannot load module: " + std::string{ RENDERDOC_DLL });
        }

        if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
        {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI =
                (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&_renderdoc_api);
            if (ret != 1)
            {
                throw std::runtime_error("Cannot open renderdoc dll");
            }
        }
        else
        {
            throw std::runtime_error("Cannot open the file handle: " + std::string{ RENDERDOC_DLL });
        }
#endif
        _renderer_factory = std::move(info.renderer_factory);

        glfwInit();
        if (isVulkanInitialized() == false)
        {
            initVulkan(info);
            createDevices(info);
        }
        _initialized = true;
    }
    void RenderContext::initVulkan(const InitializationInfo& init_info)
    {
        volkInitialize();
        if (checkLayerSupport(init_info.enabled_layers) == false)
        {
            throw std::runtime_error("Instance layers requested but not available");
        }
        if (init_info.app_info.apiVersion != VK_API_VERSION_1_3)
        {
            throw std::runtime_error("Not supported vulkan api version");
        }
        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &init_info.app_info;

        std::vector<const char*> instance_extensions = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
        {
            uint32_t glfw_extension_count = 0;
            const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
            for (uint32_t i = 0; i < glfw_extension_count; ++i)
            {
                instance_extensions.push_back(glfw_extensions[i]);
            }
            for (const auto& requested_extension : init_info.instance_extensions)
            {
                instance_extensions.push_back(requested_extension.c_str());
            }
        }

        create_info.enabledExtensionCount = instance_extensions.size();
        create_info.ppEnabledExtensionNames = instance_extensions.data();
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        std::vector<const char*> enabled_layers(init_info.enabled_layers.size(), nullptr);
        std::ranges::transform(init_info.enabled_layers, enabled_layers.begin(),
                               [](const std::string& str) { return str.c_str(); });
        create_info.enabledLayerCount = enabled_layers.size();
        create_info.ppEnabledLayerNames = enabled_layers.data();
        if (init_info.enable_validation_layers)
        {
            debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_create_info.pfnUserCallback = debugCallback;

            create_info.pNext = &debug_create_info;
        }

        if (VkResult result = vkCreateInstance(&create_info, nullptr, &_instance); result != VK_SUCCESS)
        {
            throw std::runtime_error(std::string{ "Cannot initialize vulkan instance: " } + string_VkResult(result));
        }
        volkLoadInstance(_instance);

    }

    void RenderContext::createDevices(const InitializationInfo& info)
    {
        std::vector<const char*> device_extensions{ Window::kDeviceExtensions.begin(), Window::kDeviceExtensions.end() };
        device_extensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

        std::ranges::transform(info.device_extensions,
                               std::back_inserter(device_extensions),
                               [](const std::string& name) { return name.c_str(); });
        DeviceLookup device_lookup(_instance);
        VkPhysicalDevice physical_device = info.device_selector(device_lookup);

        if (physical_device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Device selection failed");
        }
        DeviceLookup::DeviceInfo device_info = device_lookup.queryDeviceInfo(physical_device);
        InitializationInfo::QueueFamilyIndexes queue_families = info.queue_family_selector(device_info);

        {
            std::vector<const char*> enabled_layers(info.enabled_layers.size(), nullptr);
            std::ranges::transform(info.enabled_layers, enabled_layers.begin(),
                                   [](const std::string& str) { return str.c_str(); });
            auto device = std::make_unique<Device>(_instance,
                                                   physical_device,
                                                   queue_families.graphics,
                                                   queue_families.present,
                                                   queue_families.transfer,
                                                   device_extensions,
                                                   enabled_layers,
                                                   std::move(device_info));
            _devices.push_back(std::move(device));
            /* TODO implement volk device table usage
             * Volk can load functions into table, and it might be different from device to device.
             * Implement a VulkanLogicalDevice that has the api callbacks.
             */
        }
    }

    void RenderContext::reset()
    {
        if (isVulkanInitialized())
        {
            _garbage.clear();
            _devices.clear();
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