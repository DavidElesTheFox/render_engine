#include "DeviceSelector.h"

#include <iostream>
#include <ostream>

namespace
{
    std::ostream& operator<<(std::ostream& os, const RenderEngine::DeviceLookup::DeviceExtensionInfo& info)
    {
        os << "'" << info.name
            << "' version: " << (info.version != std::nullopt ? std::to_string(*info.version) : std::string{ "Not defined" });
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const RenderEngine::DeviceLookup::QueueFamilyInfo& info)
    {
        auto optional_int_to_str = [](const std::optional<uint32_t>& v)->std::string { return v != std::nullopt ? std::to_string(*v) : "Not defined"; };
        auto optional_bool_to_str = [](const std::optional<bool>& v)->std::string { return v != std::nullopt ? (*v ? "X" : " ") : "Not defined"; };

        os << "queue count: [" << optional_int_to_str(info.queue_count) << "]"
            << "; has graphics support [" << optional_bool_to_str(info.hasGraphicsSupport) << "]"
            << "; has presentation support [" << optional_bool_to_str(info.hasPresentSupport) << "]"
            << "; has transfer support [" << optional_bool_to_str(info.hasTransferSupport) << "]"
            << "; has compute support [" << optional_bool_to_str(info.hasComputeSupport) << "]";

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const RenderEngine::DeviceLookup::DeviceInfo& info)
    {
        auto optional_bool_to_str = [](const std::optional<bool>& v)->std::string { return v != std::nullopt ? (*v ? "X" : " ") : "Not defined"; };

        os << "Device: \n";
        os << "  Has Cuda support: [" << optional_bool_to_str(info.hasCudaSupport) << "]\n";
        os << "  Queue Families:\n";
        for (const auto& queue_family_info : info.queue_families)
        {
            os << "    - " << queue_family_info << "\n";
        }
        os << "  Device Extensions: " << info.device_extensions.size();

        return os;
    }
}

VkPhysicalDevice DeviceSelector::askForDevice(const RenderEngine::DeviceLookup& lookup)
{
    const auto devices = lookup.queryDevices();
    constexpr auto delimeter = "===========================================";
    for (uint32_t i = 0; i < devices.size(); ++i)
    {
        std::cout << i << ") " << devices[i] << std::endl;
        std::cout << delimeter << std::endl;
    }
    std::cout << "Selected device: " << std::flush;
    uint32_t device_id = 0;
    std::cin >> device_id;
    return devices.at(device_id).phyiscal_device;
}

RenderEngine::RenderContext::InitializationInfo::QueueFamilyIndexes
DeviceSelector::askForQueueFamilies(const RenderEngine::DeviceLookup::DeviceInfo& info)
{
    auto info_without_extensions = info;
    info_without_extensions.device_extensions.clear();
    std::cout << "Device info:" << std::endl;
    std::cout << info << std::endl;

    RenderEngine::RenderContext::InitializationInfo::QueueFamilyIndexes result{};

    std::cout << "Selected queue for graphics: " << std::flush;
    std::cin >> result.graphics;

    std::cout << "Selected queue for presentation: " << std::flush;
    std::cin >> result.present;

    std::cout << "Selected queue for transfer: " << std::flush;
    std::cin >> result.transfer;

    return result;
}
