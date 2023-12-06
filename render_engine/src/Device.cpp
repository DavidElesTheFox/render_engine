#include <render_engine/Device.h>

#include <volk.h>

#include <render_engine/GpuResourceManager.h>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <render_engine/resources/Texture.h>
#include <render_engine/TransferEngine.h>
#include <render_engine/window/OffScreenWindow.h>
#include <render_engine/window/Window.h>

#include <GLFW/glfw3native.h>

#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_win32.h>
namespace
{
    // TODO support multiple queue count
    constexpr size_t k_supported_queue_count = 1;

    VkDevice createLogicalDevice(size_t queue_count,
                                 VkPhysicalDevice physical_device,
                                 uint32_t queue_family_index_graphics,
                                 uint32_t queue_family_index_presentation,
                                 const std::vector<const char*>& device_extensions,
                                 const std::vector<const char*>& validation_layers)
    {

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = { queue_family_index_graphics, queue_family_index_presentation };

        std::vector<float> queuePriority(queue_count, 1.0f);
        for (uint32_t queueFamily : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queueFamily;
            queue_create_info.queueCount = queue_count;
            queue_create_info.pQueuePriorities = queuePriority.data();
            queue_create_infos.push_back(queue_create_info);
        }

        {
            VkPhysicalDeviceSynchronization2Features synchronization_2_feature{};
            synchronization_2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
            //synchronization_2_feature.synchronization2 = true;

            VkPhysicalDeviceFeatures2KHR features =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
                .pNext = &synchronization_2_feature,
            };

            vkGetPhysicalDeviceFeatures2KHR(physical_device, &features);
            if (synchronization_2_feature.synchronization2 == false)
            {
                throw std::runtime_error("synchronization2 feature is not supported");
            }
        }

        VkPhysicalDeviceSynchronization2Features synchronization_2_feature{};
        synchronization_2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        synchronization_2_feature.synchronization2 = true;

        VkPhysicalDeviceFeatures2 device_features{};
        device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        device_features.pNext = &synchronization_2_feature;

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pNext = &device_features;

        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.queueCreateInfoCount = queue_create_infos.size();

        create_info.pEnabledFeatures = VK_NULL_HANDLE;

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
        if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }
        volkLoadDevice(device);
        return device;

    }
}

namespace RenderEngine
{
    Device::Device(VkInstance instance,
                   VkPhysicalDevice physical_device,
                   uint32_t queue_family_index_graphics,
                   uint32_t queue_family_index_presentation,
                   uint32_t queue_family_index_transfer,
                   const std::vector<const char*>& device_extensions,
                   const std::vector<const char*>& validation_layers)
        : _instance(instance)
        , _physical_device(physical_device)
        , _logical_device(createLogicalDevice(k_supported_queue_count, physical_device, queue_family_index_graphics, queue_family_index_presentation, device_extensions, validation_layers))
        , _queue_family_present(queue_family_index_presentation)
        , _queue_family_graphics(queue_family_index_graphics)
        , _queue_family_transfer(queue_family_index_transfer)
    {}

    Device::~Device()
    {
        destroy();
    }
    void Device::destroy() noexcept
    {
        vkDestroyDevice(_logical_device, nullptr);
    }
    std::unique_ptr<Window> Device::createWindow(std::string_view name, uint32_t back_buffer_size)
    {
        VkQueue render_queue;
        VkQueue present_queue;
        vkGetDeviceQueue(_logical_device, _queue_family_graphics, 0, &render_queue);
        vkGetDeviceQueue(_logical_device, _queue_family_present, 0, &present_queue);

        constexpr auto width = 1024;
        constexpr auto height = 764;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);

        VkWin32SurfaceCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.hwnd = glfwGetWin32Window(window);
        create_info.hinstance = GetModuleHandle(nullptr);
        VkSurfaceKHR surface;

        if (vkCreateWin32SurfaceKHR(_instance, &create_info, nullptr, &surface) != VK_SUCCESS)
        {
            glfwDestroyWindow(window);
            throw std::runtime_error("Failed to create window surface!");
        }
        try
        {
            std::unique_ptr<SwapChain> swap_chain = std::make_unique<SwapChain>(SwapChain::CreateInfo{ window, _instance, _physical_device, _logical_device, std::move(surface), _queue_family_graphics, _queue_family_present, back_buffer_size });
            std::unique_ptr<RenderEngine> render_engine = createRenderEngine(back_buffer_size);
            std::unique_ptr<TransferEngine> transfer_engine = createTransferEngine();
            return std::make_unique<Window>(*this, std::move(render_engine), std::move(transfer_engine), window, std::move(swap_chain), present_queue, back_buffer_size);
        }
        catch (const std::runtime_error& error)
        {
            glfwDestroyWindow(window);
            throw;
        }
    }

    std::unique_ptr<OffScreenWindow> Device::createOffScreenWindow(std::string_view name, size_t back_buffer_size)
    {
        VkQueue render_queue;
        VkQueue present_queue;
        vkGetDeviceQueue(_logical_device, _queue_family_graphics, 0, &render_queue);
        vkGetDeviceQueue(_logical_device, _queue_family_present, 0, &present_queue);

        constexpr auto width = 1024;
        constexpr auto height = 764;



        std::unique_ptr<RenderEngine> render_engine = createRenderEngine(back_buffer_size);
        std::unique_ptr<TransferEngine> transfer_engine = createTransferEngine();
        TextureFactory texture_factory(*transfer_engine,
                                       { render_engine->getQueueFamilyIndex(), transfer_engine->getQueueFamilyIndex() },
                                       _physical_device,
                                       _logical_device);
        Image render_target_image(width, height, VK_FORMAT_R8G8B8A8_SRGB);
        std::vector<std::unique_ptr<Texture>> render_target_textures;
        for (size_t i = 0; i < back_buffer_size; ++i)
        {
            render_target_textures.push_back(texture_factory.createNoUpload(render_target_image,
                                                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                                                            VK_SHADER_STAGE_ALL,
                                                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
        }
        return std::make_unique<OffScreenWindow>(*this,
                                                 std::move(render_engine),
                                                 std::move(transfer_engine),
                                                 std::move(render_target_textures));
    }

    std::unique_ptr<RenderEngine> Device::createRenderEngine(size_t back_buffer_size)
    {
        VkQueue render_queue;
        vkGetDeviceQueue(_logical_device, _queue_family_graphics, 0, &render_queue);

        return std::make_unique<RenderEngine>(*this, render_queue, _queue_family_graphics, back_buffer_size);
    }

    std::unique_ptr<TransferEngine> Device::createTransferEngine()
    {
        VkQueue transfer_queue;
        vkGetDeviceQueue(_logical_device, _queue_family_transfer, 0, &transfer_queue);

        return std::make_unique<TransferEngine>(_logical_device, _queue_family_transfer, transfer_queue);
    }

    std::unique_ptr<TextureFactory> Device::createTextureFactory(TransferEngine& transfer_engine, std::set<uint32_t> compatible_queue_family_indexes)
    {
        return std::make_unique<TextureFactory>(transfer_engine,
                                                std::move(compatible_queue_family_indexes),
                                                _physical_device,
                                                _logical_device);
    }

}