#pragma once

#include <render_engine/RenderContext.h>

class DeviceSelector
{
public:
    explicit DeviceSelector(bool multiple_device_selection_enabled = false)
        : _multiple_device_selection_enabled(multiple_device_selection_enabled)
    {}

    VkPhysicalDevice askForDevice(const RenderEngine::DeviceLookup&);
    RenderEngine::RenderContext::InitializationInfo::QueueFamilyIndexes
        askForQueueFamilies(const RenderEngine::DeviceLookup::DeviceInfo&);
private:
    bool _multiple_device_selection_enabled{ false };
    uint32_t _selected_device_count{ 0 };
};