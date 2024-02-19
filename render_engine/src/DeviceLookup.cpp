#include <render_engine/DeviceLookup.h>

#include <GLFW/glfw3.h>

#include <stdexcept>

#include <render_engine/cuda_compute/CudaDevice.h>

namespace RenderEngine
{

    namespace
    {
        struct DeviceProperties
        {
            VkPhysicalDeviceIDProperties id;
            VkPhysicalDeviceProperties general;
        };
        DeviceProperties getDeviceProperties(VkPhysicalDevice physical_device)
        {
            VkPhysicalDeviceIDProperties device_id_property = {};
            device_id_property.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
            device_id_property.pNext = nullptr;
            VkPhysicalDeviceProperties2 device_property = {};
            device_property.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            device_property.pNext = &device_id_property;
            vkGetPhysicalDeviceProperties2(physical_device, &device_property);
            return { device_id_property, device_property.properties };
        }
    }
    std::vector<DeviceLookup::DeviceInfo> DeviceLookup::queryDevices() const
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);
        if (device_count == 0)
        {
            throw std::runtime_error("failed to find any GPU with vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        if (vkEnumeratePhysicalDevices(_instance, &device_count, devices.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot access to device properties");
        }

        std::vector<DeviceLookup::DeviceInfo> result;
        for (auto physical_device : devices)
        {
            result.emplace_back(queryDeviceInfo(physical_device));
        }
        return result;
    }

    DeviceLookup::DeviceInfo DeviceLookup::queryDeviceInfo(VkPhysicalDevice physical_device) const
    {
        DeviceLookup::DeviceInfo result{};
        result.phyiscal_device = physical_device;
        result.queue_families = queryQueueFamilies(physical_device);
        result.device_extensions = queryDeviceExtensions(physical_device);
        auto device_info = getDeviceProperties(physical_device);
        result.name = device_info.general.deviceName;
        result.hasCudaSupport = CudaCompute::CudaDevice::hasDeviceWithUUIDCudaSupport(std::span{ &device_info.id.deviceUUID[0], VK_UUID_SIZE });
        return result;
    }


    std::vector<DeviceLookup::QueueFamilyInfo> DeviceLookup::queryQueueFamilies(VkPhysicalDevice device) const
    {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
        std::vector<DeviceLookup::QueueFamilyInfo> result(queue_family_count, DeviceLookup::QueueFamilyInfo{});
        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
            DeviceLookup::QueueFamilyInfo info{};
            const auto& queue_family = queue_families[i];

            info.hasComputeSupport = queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT;
            info.hasGraphicsSupport = queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            info.hasTransferSupport = queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT;
            info.hasPresentSupport = glfwGetPhysicalDevicePresentationSupport(_instance, device, i);
            info.queue_count = queue_family.queueCount;
            result[i] = (std::move(info));
        }

        return result;
    }
    std::vector<DeviceLookup::DeviceExtensionInfo> DeviceLookup::queryDeviceExtensions(VkPhysicalDevice physical_device) const
    {
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());
        std::vector<DeviceLookup::DeviceExtensionInfo> result;
        for (const auto& extension : available_extensions)
        {
            result.emplace_back();
            result.back().name = extension.extensionName;
            result.back().version = extension.specVersion;
        }
        return result;
    }

}