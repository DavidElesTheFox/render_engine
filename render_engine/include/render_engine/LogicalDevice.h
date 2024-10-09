#pragma once

#include <volk.h>

#include <algorithm>

namespace RenderEngine
{
    class LogicalDevice
    {
    public:
        explicit LogicalDevice(VkDevice device);

        ~LogicalDevice();
        LogicalDevice(const LogicalDevice&) = delete;
        LogicalDevice(LogicalDevice&& o) = delete;

        LogicalDevice& operator=(const LogicalDevice&) = delete;
        LogicalDevice& operator=(LogicalDevice&& o) = delete;

        VolkDeviceTable* operator->() { return &_api; }
        VkDevice operator*() { return _logical_device; }
    private:
        VkDevice _logical_device{ VK_NULL_HANDLE };
        VolkDeviceTable _api{ };
    };
}