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

    void Buffer::uploadUnmapped(std::span<const uint8_t> data_view, TransferEngine& transfer_engine, CommandContext* dst_context, SyncOperations sync_operations)
    {
        assert(isMapped() == false);
        if (_buffer_info.size != data_view.size())
        {
            throw std::runtime_error("Invalid size during upload. Buffer size: " + std::to_string(_buffer_info.size) + " data to upload: " + std::to_string(data_view.size()));
        }
        std::vector<SyncObject> result;
        auto [staging_buffer, staging_memory] = createBuffer(_physical_device,
                                                             _logical_device,
                                                             _buffer_info.size,
                                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data = nullptr;
        vkMapMemory(_logical_device, staging_memory, 0, _buffer_info.size, 0, &data);
        memcpy(data, data_view.data(), (size_t)_buffer_info.size);
        vkUnmapMemory(_logical_device, staging_memory);

        const bool is_initial_transfer = _buffer_state.command_context.expired();
        if (is_initial_transfer)
        {
            _buffer_state.command_context = transfer_engine.getTransferContext().getWeakReference();
        }
        auto* src_unit = _buffer_state.command_context.lock().get();

        if (src_unit->getQueue() != dst_context->getQueue())
        {
            // TODO remove lock
            SyncObject global_object = SyncObject::CreateWithFence(src_unit->getLogicalDevice(), 0);


            SyncObject transfer_sync_object = SyncObject::CreateEmpty(src_unit->getLogicalDevice());
            transfer_sync_object.createSemaphore("DataTransferFinished");
            transfer_sync_object.addSignalOperationToGroup(SyncGroups::kInternal, "DataTransferFinished", VK_PIPELINE_STAGE_2_TRANSFER_BIT);
            transfer_sync_object.addWaitOperationToGroup(SyncGroups::kExternal, "DataTransferFinished", VK_PIPELINE_STAGE_2_TRANSFER_BIT);
            auto upload_command = [&](VkCommandBuffer command_buffer)
                {
                    ResourceStateMachine state_machine;
                    state_machine.recordStateChange(this,
                                                    getResourceState().clone()
                                                    .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                    .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT));
                    VkBufferCopy copy_region{};
                    copy_region.size = _buffer_info.size;
                    vkCmdCopyBuffer(command_buffer, staging_buffer, _buffer, 1, &copy_region);
                };
            if (is_initial_transfer == false)
            {
                SyncObject sync_object_src_to_transfer = ResourceStateMachine::transferOwnership(this,
                                                                                                 getResourceState().clone()
                                                                                                 .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                                                                 .setAccessFlag(0),
                                                                                                 _buffer_state.command_context.lock().get(),
                                                                                                 &transfer_engine.getTransferContext(),
                                                                                                 sync_operations.extract(SyncOperations::ExtractWaitOperations));
                transfer_engine.transfer(sync_object_src_to_transfer.getOperationsGroup(SyncGroups::kExternal)
                                         .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
                                         upload_command);
                result.push_back(std::move(sync_object_src_to_transfer));

            }
            else
            {
                SyncObject sync_object_src_to_transfer = SyncObject::CreateWithFence(src_unit->getLogicalDevice(), 0);
                transfer_engine.transfer(sync_object_src_to_transfer.getOperationsGroup(SyncGroups::kExternal)
                                         .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
                                         upload_command);
                result.push_back(std::move(sync_object_src_to_transfer));
            }
            assert(getResourceState().getQueueFamilyIndex() == transfer_engine.getTransferContext().getQueueFamilyIndex());

            SyncObject sync_object_transfer_to_dst = ResourceStateMachine::transferOwnership(this,
                                                                                             getResourceState().clone(),
                                                                                             _buffer_state.command_context.lock().get(),
                                                                                             dst_context,
                                                                                             sync_operations.extract(SyncOperations::ExtractSignalOperations)
                                                                                             .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kExternal))
                                                                                             .createUnionWith(global_object.getOperationsGroup(SyncGroups::kInternal)));
            assert(getResourceState().getQueueFamilyIndex() == dst_context->getQueueFamilyIndex());
            result.push_back(std::move(sync_object_transfer_to_dst));
            result.push_back(std::move(transfer_sync_object));
            vkWaitForFences(_logical_device, 1, global_object.getOperationsGroup(SyncGroups::kInternal).getFence(), VK_TRUE, UINT64_MAX);

            vkDestroyBuffer(_logical_device, staging_buffer, nullptr);
            vkFreeMemory(_logical_device, staging_memory, nullptr);
        }
        else
        {
            // TODO remove global lock
            SyncObject sync_object = SyncObject::CreateWithFence(_logical_device, 0);
            transfer_engine.transfer(sync_object.getOperationsGroup(SyncGroups::kInternal),
                                     [&](VkCommandBuffer command_buffer)
                                     {
                                         VkBufferCopy copy_region{};
                                         copy_region.size = _buffer_info.size;
                                         vkCmdCopyBuffer(command_buffer, staging_buffer, _buffer, 1, &copy_region);
                                     });

            vkWaitForFences(_logical_device, 1, sync_object.getOperationsGroup(SyncGroups::kInternal).getFence(), VK_TRUE, UINT64_MAX);
            // TODO remove lock

            vkDestroyBuffer(_logical_device, staging_buffer, nullptr);
            vkFreeMemory(_logical_device, staging_memory, nullptr);
        }
    }

    void Buffer::uploadMapped(std::span<const uint8_t> data_view)
    {
        assert(isMapped());
        memcpy(_mapped_memory, data_view.data(), data_view.size());
    }

}