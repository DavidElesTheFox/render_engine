#pragma once

#include <volk.h>

#include <cstdint>
#include <memory>
#include <stdexcept>

namespace RenderEngine
{
    class Buffer;

    class GpuResourceManager
    {
    public:
        GpuResourceManager(VkPhysicalDevice physical_device,
                           VkDevice logical_device,
                           uint32_t back_buffer_size,
                           uint32_t max_num_of_resources);

        GpuResourceManager(const GpuResourceManager&) = delete;
        GpuResourceManager(GpuResourceManager&&) = delete;
        GpuResourceManager operator=(const GpuResourceManager&) = delete;
        GpuResourceManager operator=(GpuResourceManager&&) = delete;
        ~GpuResourceManager();
        std::unique_ptr<Buffer> createAttributeBuffer(VkBufferUsageFlags usage, VkDeviceSize size);
        std::unique_ptr<Buffer> createUniformBuffer(VkDeviceSize size);
        VkDescriptorPool getDescriptorPool() { return _descriptor_pool; }
        VkDevice getLogicalDevice() const { return _logical_device; }
        VkPhysicalDevice getPhysicalDevice() const { return _physical_device; }
        uint32_t getBackBufferSize() const { return _back_buffer_size; }
    private:
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        VkDevice _logical_device{ VK_NULL_HANDLE };
        VkDescriptorPool _descriptor_pool{ VK_NULL_HANDLE };
        uint32_t _back_buffer_size{ 1 };
    };
}
