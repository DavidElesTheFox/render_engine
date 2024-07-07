#pragma once

#include <volk.h>

#include <cstdint>
#include <memory>
#include <stdexcept>

#include <render_engine/LogicalDevice.h>

namespace RenderEngine
{
    class Buffer;
    class CoherentBuffer;

    class GpuResourceManager
    {
    public:
        GpuResourceManager(VkPhysicalDevice physical_device,
                           LogicalDevice& logical_device,
                           uint32_t back_buffer_size,
                           uint32_t max_num_of_resources);

        GpuResourceManager(const GpuResourceManager&) = delete;
        GpuResourceManager(GpuResourceManager&&) = delete;
        GpuResourceManager operator=(const GpuResourceManager&) = delete;
        GpuResourceManager operator=(GpuResourceManager&&) = delete;
        ~GpuResourceManager();
        std::unique_ptr<Buffer> createAttributeBuffer(VkBufferUsageFlags usage, VkDeviceSize size);
        std::unique_ptr<CoherentBuffer> createUniformBuffer(VkDeviceSize size);
        VkDescriptorPool getDescriptorPool() { return _descriptor_pool; }
        LogicalDevice& getLogicalDevice() const { return _logical_device; }
        VkPhysicalDevice getPhysicalDevice() const { return _physical_device; }

        // TODO: remove this function.
        uint32_t getBackBufferSize() const { return _back_buffer_size; }
    private:
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice& _logical_device;
        VkDescriptorPool _descriptor_pool{ VK_NULL_HANDLE };
        uint32_t _back_buffer_size{ 1 };
    };
}
