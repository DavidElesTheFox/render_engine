#pragma once

#include <render_engine/RenderContext.h>

class DeviceSelector
{
public:
    DeviceSelector() = default;

    VkPhysicalDevice askForDevice(const RenderEngine::DeviceLookup&);
    RenderEngine::RenderContext::InitializationInfo::QueueFamilyIndexes
        askForQueueFamilies(const RenderEngine::DeviceLookup::DeviceInfo&);
};