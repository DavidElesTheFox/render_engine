#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include <volk.h>

namespace RenderEngine
{
    class DeviceLookup
    {
    public:
        struct QueueFamilyInfo
        {
            std::optional<uint32_t> queue_count;
            std::optional<bool> hasGraphicsSupport;
            std::optional<bool> hasComputeSupport;
            std::optional<bool> hasTransferSupport;
            std::optional<bool> hasPresentSupport;

            bool isMatching(const QueueFamilyInfo& o) const
            {
                return queue_count >= o.queue_count
                    && (o.hasGraphicsSupport == std::nullopt || hasGraphicsSupport == o.hasGraphicsSupport)
                    && (o.hasComputeSupport == std::nullopt || hasComputeSupport == o.hasComputeSupport)
                    && (o.hasTransferSupport == std::nullopt || hasTransferSupport == o.hasTransferSupport)
                    && (o.hasPresentSupport == std::nullopt || hasPresentSupport == o.hasPresentSupport);
            }
        };
        struct DeviceExtensionInfo
        {
            std::string name;
            std::optional<uint32_t> version;

            bool isMatching(const DeviceExtensionInfo& o) const
            {
                return name == o.name && version >= o.version;
            }
        };
        struct DeviceInfo
        {
            std::vector<QueueFamilyInfo> queue_families;
            std::vector<DeviceExtensionInfo> device_extensions;
            std::optional<bool> hasCudaSupport;
            VkPhysicalDevice phyiscal_device{ VK_NULL_HANDLE };

            bool isMatching(const DeviceInfo& o) const
            {
                return std::all_of(queue_families.begin(), queue_families.end(),
                                   [&](const QueueFamilyInfo& a)
                                   {
                                       return std::any_of(o.queue_families.begin(), o.queue_families.end(),
                                       [&](const QueueFamilyInfo& b) { return a.isMatching(b); });
                                   }) &&
                    std::all_of(device_extensions.begin(), device_extensions.end(),
                                [&](const DeviceExtensionInfo& a)
                                {
                                    return std::any_of(o.device_extensions.begin(), o.device_extensions.end(),
                                    [&](const DeviceExtensionInfo& b) { return a.isMatching(b); });
                                }) &&
                                       (o.hasCudaSupport == std::nullopt || hasCudaSupport == o.hasCudaSupport)
                                    && (o.phyiscal_device == VK_NULL_HANDLE || phyiscal_device == o.phyiscal_device);
            }
        };

        explicit DeviceLookup(VkInstance instance) : _instance(instance)
        {};

        std::vector<DeviceInfo> queryDevices() const;

        DeviceInfo queryDeviceInfo(VkPhysicalDevice physical_device) const;
    private:

        std::vector<QueueFamilyInfo> queryQueueFamilies(VkPhysicalDevice) const;
        std::vector<DeviceExtensionInfo> queryDeviceExtensions(VkPhysicalDevice) const;
        VkInstance _instance{ VK_NULL_HANDLE };
    };
}