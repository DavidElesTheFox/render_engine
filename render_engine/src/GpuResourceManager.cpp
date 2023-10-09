#include <render_engine/GpuResourceManager.h>

#include <render_engine/resources/Buffer.h>

namespace RenderEngine
{
	GpuResourceManager::GpuResourceManager(VkPhysicalDevice physical_device, VkDevice logical_device,
		uint32_t back_buffer_size, uint32_t max_num_of_resources) : _physical_device(physical_device)
		, _logical_device(logical_device)
		, _back_buffer_size(back_buffer_size)
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = back_buffer_size * max_num_of_resources;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = back_buffer_size * max_num_of_resources;


		if (vkCreateDescriptorPool(logical_device, &poolInfo, nullptr, &_descriptor_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	GpuResourceManager::~GpuResourceManager()
	{
		vkDestroyDescriptorPool(_logical_device, _descriptor_pool, nullptr);
	}



	std::unique_ptr<Buffer> GpuResourceManager::createAttributeBuffer(VkBufferUsageFlags usage, VkDeviceSize size)
	{
		return Buffer::CreateAttributeBuffer(_physical_device, _logical_device, usage, size);
	}
	std::unique_ptr<Buffer> GpuResourceManager::createUniformBuffer(VkDeviceSize size)
	{
		return Buffer::CreateUniformBuffer(_physical_device, _logical_device, size);
	}
}
