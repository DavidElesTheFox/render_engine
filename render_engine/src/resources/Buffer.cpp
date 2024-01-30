#include <render_engine/resources/Buffer.h>

#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/TransferEngine.h>

#include <cassert>
#include <map>
#include <stdexcept>
#include <string>
namespace
{

    uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
    std::pair<VkBuffer, VkDeviceMemory> createBuffer(
        VkPhysicalDevice physical_device,
        VkDevice logical_device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties)
    {
        VkBuffer buffer;
        VkDeviceMemory buffer_memory;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logical_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(logical_device, buffer, &mem_requirements);

        VkMemoryAllocateInfo allocInfo{};

        try
        {
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = mem_requirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties);

            if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &buffer_memory) != VK_SUCCESS)
            {
                vkDestroyBuffer(logical_device, buffer, nullptr);
                throw std::runtime_error("failed to allocate buffer memory!");
            }

        }
        catch (const std::runtime_error&)
        {
            vkDestroyBuffer(logical_device, buffer, nullptr);
            throw;
        }

        vkBindBufferMemory(logical_device, buffer, buffer_memory, 0);
        return { buffer, buffer_memory };
    }
}

namespace RenderEngine
{
    Buffer::Buffer(VkPhysicalDevice physical_device,
                   VkDevice logical_device,
                   BufferInfo&& buffer_info)
        : _physical_device(physical_device)
        , _logical_device(logical_device)
        , _buffer_info(std::move(buffer_info))
    {
        std::tie(_buffer, _buffer_memory) = createBuffer(_physical_device,
                                                         _logical_device,
                                                         _buffer_info.size,
                                                         _buffer_info.usage,
                                                         _buffer_info.memory_properties);
        if (isMapped())
        {
            vkMapMemory(_logical_device, _buffer_memory, 0, getDeviceSize(), 0, &_mapped_memory);
        }
    }

    Buffer::~Buffer()
    {
        vkDestroyBuffer(_logical_device, _buffer, nullptr);
        vkFreeMemory(_logical_device, _buffer_memory, nullptr);
    }

    void Buffer::uploadUnmapped(std::span<const uint8_t> data_view, TransferEngine& transfer_engine, uint32_t dst_queue_index)
    {
        assert(isMapped() == false);
        if (_buffer_info.size != data_view.size())
        {
            throw std::runtime_error("Invalid size during upload. Buffer size: " + std::to_string(_buffer_info.size) + " data to upload: " + std::to_string(data_view.size()));
        }
        auto [staging_buffer, staging_memory] = createBuffer(_physical_device,
                                                             _logical_device,
                                                             _buffer_info.size,
                                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data = nullptr;
        vkMapMemory(_logical_device, staging_memory, 0, _buffer_info.size, 0, &data);
        memcpy(data, data_view.data(), (size_t)_buffer_info.size);
        vkUnmapMemory(_logical_device, staging_memory);

        if (_buffer_state.queue_family_index == std::nullopt)
        {
            _buffer_state.queue_family_index = transfer_engine.getQueueFamilyIndex();
        }
        SyncObject sync_object = SyncObject::CreateWithFence(_logical_device, 0);
        auto inflight_data = transfer_engine.transfer(sync_object.getOperationsGroup(SyncGroups::kInternal),
                                                      [&](VkCommandBuffer command_buffer)
                                                      {
                                                          ResourceStateMachine state_machine;

                                                          state_machine.recordStateChange(this,
                                                                                          getResourceState().clone().setQueueFamilyIndex(transfer_engine.getQueueFamilyIndex()));
                                                          state_machine.commitChanges(command_buffer);

                                                          VkBufferCopy copy_region{};
                                                          copy_region.size = _buffer_info.size;
                                                          vkCmdCopyBuffer(command_buffer, staging_buffer, _buffer, 1, &copy_region);

                                                          state_machine.recordStateChange(this,
                                                                                          getResourceState().clone().setQueueFamilyIndex(dst_queue_index));
                                                          state_machine.commitChanges(command_buffer);
                                                      });

        vkWaitForFences(_logical_device, 1, sync_object.getOperationsGroup(SyncGroups::kInternal).getFence(), VK_TRUE, UINT64_MAX);

        vkDestroyBuffer(_logical_device, staging_buffer, nullptr);
        vkFreeMemory(_logical_device, staging_memory, nullptr);
    }

    void Buffer::uploadMapped(std::span<const uint8_t> data_view)
    {
        assert(isMapped());
        memcpy(_mapped_memory, data_view.data(), data_view.size());
    }

}