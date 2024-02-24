#include <render_engine/GpuResourceManager.h>

#include <render_engine/resources/Buffer.h>

#include <array>
namespace RenderEngine
{

    namespace
    {
        BufferInfo createBufferInfoForAttributeBuffer(VkBufferUsageFlags usage, VkDeviceSize size)
        {
            return { usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
        }
        BufferInfo createBufferInfoForUniformBuffer(VkDeviceSize size)
        {
            return { VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        }
    }
    GpuResourceManager::GpuResourceManager(VkPhysicalDevice physical_device,
                                           LogicalDevice& logical_device,
                                           uint32_t back_buffer_size,
                                           uint32_t max_num_of_resources)
        : _physical_device(physical_device)
        , _logical_device(logical_device)
        , _back_buffer_size(back_buffer_size)
    {
        std::array<VkDescriptorPoolSize, 2> pool_sizes;

        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = back_buffer_size * max_num_of_resources;

        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount = back_buffer_size * max_num_of_resources;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = pool_sizes.size();
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = back_buffer_size * max_num_of_resources;


        if (_logical_device->vkCreateDescriptorPool(*_logical_device, &poolInfo, nullptr, &_descriptor_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    GpuResourceManager::~GpuResourceManager()
    {
        _logical_device->vkDestroyDescriptorPool(*_logical_device, _descriptor_pool, nullptr);
    }

    std::unique_ptr<Buffer> GpuResourceManager::createAttributeBuffer(VkBufferUsageFlags usage, VkDeviceSize size)
    {
        return std::make_unique<Buffer>(_physical_device, _logical_device, createBufferInfoForAttributeBuffer(usage, size));
    }
    std::unique_ptr<CoherentBuffer> GpuResourceManager::createUniformBuffer(VkDeviceSize size)
    {
        return std::make_unique<CoherentBuffer>(_physical_device, _logical_device, createBufferInfoForUniformBuffer(size));
    }
}
