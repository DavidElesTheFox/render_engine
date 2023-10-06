#pragma once

#include <render_engine/memory//Buffer.h>

#include <volk.h>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace RenderEngine
{
	class Window;
	class GpuResourceManager
	{
	public:
		GpuResourceManager(VkPhysicalDevice physical_device,
			VkDevice logical_device,
			uint32_t back_buffer_size,
			uint32_t max_num_of_resources)
			: _physical_device(physical_device)
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

		GpuResourceManager(const GpuResourceManager&) = delete;
		GpuResourceManager(GpuResourceManager&&) = delete;
		GpuResourceManager operator=(const GpuResourceManager&) = delete;
		GpuResourceManager operator=(GpuResourceManager&&) = delete;
		~GpuResourceManager();
		std::unique_ptr<Buffer> createAttributeBuffer(VkBufferUsageFlags usage, VkDeviceSize size);
		std::unique_ptr<Buffer> createUniformBuffer(VkDeviceSize size);
		VkDescriptorPool descriptorPool() { return _descriptor_pool; }
		VkDevice getLogicalDevice() { return _logical_device; }
		uint32_t getBackBufferSize() const { return _back_buffer_size; }
	private:
		VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
		VkDevice _logical_device{ VK_NULL_HANDLE };
		VkDescriptorPool _descriptor_pool{ VK_NULL_HANDLE };
		uint32_t _back_buffer_size{ 1 };
	};

	class RenderEngine
	{
	public:
		static constexpr auto kSupportedWindowCount = 1;

		explicit RenderEngine(VkInstance instance,
			VkPhysicalDevice physical_device,
			uint32_t queue_family_index_graphics,
			uint32_t queue_family_index_presentation,
			const std::vector<const char*>& device_extensions,
			const std::vector<const char*>& validation_layers);
		RenderEngine(const RenderEngine&) = delete;
		RenderEngine(RenderEngine&&) = delete;

		RenderEngine& operator=(const RenderEngine&) = delete;
		RenderEngine& operator=(RenderEngine&&) = delete;

		~RenderEngine();
		std::unique_ptr<Window> createWindow(std::string_view name, size_t back_buffer_size);


		VkDevice getLogicalDevice() { return _logical_device; }
		VkPhysicalDevice getPhysicalDevice() { return _physical_device; }
		VkInstance& getVulkanInstance() { return _instance; }
	private:
		VkInstance _instance;
		VkPhysicalDevice _physical_device;
		VkDevice _logical_device;
		uint32_t _queue_family_present = 0;
		uint32_t _queue_family_graphics = 0;
		uint32_t _next_queue_index = 0;
	};
}
