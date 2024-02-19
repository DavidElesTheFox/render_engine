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
        LogicalDevice(LogicalDevice&& o)
        {
            using std::swap;
            *this = std::move(o);
        }

        LogicalDevice& operator=(const LogicalDevice&) = delete;
        LogicalDevice& operator=(LogicalDevice&& o)
        {
            using std::swap;
            swap(_api, o._api);
            swap(_logical_device, o._logical_device);
            return *this;
        }

        VolkDeviceTable* operator->() { return &_api; }
        VkDevice operator*() { return _logical_device; }
    private:
        VkDevice _logical_device{ VK_NULL_HANDLE };
        VolkDeviceTable _api{ };
    };
}