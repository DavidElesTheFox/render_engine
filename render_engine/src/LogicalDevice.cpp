#include <render_engine/LogicalDevice.h>

#include <cassert>

namespace RenderEngine
{
    LogicalDevice::LogicalDevice(VkDevice logical_device)
        : _logical_device(logical_device)
    {
        assert(logical_device != VK_NULL_HANDLE);
        volkLoadDeviceTable(&_api, _logical_device);
    }
    LogicalDevice::~LogicalDevice()
    {
        if (_logical_device)
        {
            vkDestroyDevice(_logical_device, nullptr);
        }
    }
}