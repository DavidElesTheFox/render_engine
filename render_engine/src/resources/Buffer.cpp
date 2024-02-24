#include <render_engine/resources/Buffer.h>

#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/TransferEngine.h>

#include <cassert>
#include <map>
#include <stdexcept>
#include <string>

namespace RenderEngine
{
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
            LogicalDevice& logical_device,
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

            if (logical_device->vkCreateBuffer(*logical_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer!");
            }

            VkMemoryRequirements mem_requirements;
            logical_device->vkGetBufferMemoryRequirements(*logical_device, buffer, &mem_requirements);

            VkMemoryAllocateInfo allocInfo{};

            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = mem_requirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties);

            if (logical_device->vkAllocateMemory(*logical_device, &allocInfo, nullptr, &buffer_memory) != VK_SUCCESS)
            {
                logical_device->vkDestroyBuffer(*logical_device, buffer, nullptr);
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            logical_device->vkBindBufferMemory(*logical_device, buffer, buffer_memory, 0);
            return { buffer, buffer_memory };
        }
    }

    Buffer::Buffer(VkPhysicalDevice physical_device,
                   LogicalDevice& logical_device,
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
            _logical_device->vkMapMemory(*_logical_device, _buffer_memory, 0, getDeviceSize(), 0, &_mapped_memory);
        }
    }

    Buffer::~Buffer()
    {
        assert(_ongoing_upload == nullptr || _ongoing_upload->isFinished());
        assert(_ongoing_download == nullptr || _ongoing_download->isFinished());
        _logical_device->vkDestroyBuffer(*_logical_device, _buffer, nullptr);
        _logical_device->vkFreeMemory(*_logical_device, _buffer_memory, nullptr);
    }

    void Buffer::assignUploadTask(std::shared_ptr<UploadTask> task)
    {
        if (_ongoing_upload != nullptr && _ongoing_upload->isFinished() == false)
        {
            throw std::runtime_error("Buffer upload is still ongoing. New task shouldn't be started");
        }
        _ongoing_upload = task;
    }

    void Buffer::assignDownloadTask(std::shared_ptr<DownloadTask> task)
    {
        if (_ongoing_download != nullptr)
        {
            throw std::runtime_error("Buffer download is still ongoing. New task shouldn't be started");
        }
        _ongoing_download = task;
    }

    void Buffer::setInitialCommandContext(std::weak_ptr<CommandContext> command_context)
    {
        if (_buffer_state.command_context.expired() == false)
        {
            throw std::runtime_error("Buffer has a command context which shouldn't be overwritten");
        }
        _buffer_state.command_context = command_context;
    }

    void Buffer::uploadMapped(std::span<const uint8_t> data_view)
    {
        assert(isMapped());
        memcpy(_mapped_memory, data_view.data(), data_view.size());
    }

    std::shared_ptr<DownloadTask> Buffer::clearDownloadTask()
    {
        auto result = _ongoing_download;
        _ongoing_download.reset();
        return result;
    }

}