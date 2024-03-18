#include <render_engine/DataTransferScheduler.h>

#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

#include <iostream>

namespace RenderEngine
{
    namespace
    {
#pragma region Buffer Staging Memory

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
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            logical_device->vkBindBufferMemory(*logical_device, buffer, buffer_memory, 0);
            return { std::move(buffer), std::move(buffer_memory) };
        }
#pragma endregion

#pragma region Texture Upload/Download Commands
        std::function<void(VkCommandBuffer)> createTextureUnifiedUploadCommand(Texture& texture,
                                                                               SingleShotCommandContext& dst_context,
                                                                               TextureState final_state)
        {
            auto upload_command = [&](VkCommandBuffer command_buffer)
                {
                    ResourceStateMachine state_machine(dst_context.getLogicalDevice());
                    state_machine.recordStateChange(&texture,
                                                    texture.getResourceState().clone()
                                                    .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                    .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT)
                                                    .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                    );
                    state_machine.commitChanges(command_buffer);

                    {
                        VkBufferImageCopy copy_region{};
                        copy_region.bufferOffset = 0;
                        copy_region.bufferRowLength = 0;
                        copy_region.bufferImageHeight = 0;

                        copy_region.imageSubresource.aspectMask = texture.getAspect();
                        copy_region.imageSubresource.mipLevel = 0;
                        copy_region.imageSubresource.baseArrayLayer = 0;
                        copy_region.imageSubresource.layerCount = 1;

                        copy_region.imageOffset = { 0, 0, 0 };
                        copy_region.imageExtent = {
                            .width = texture.getImage().getWidth(),
                            .height = texture.getImage().getHeight(),
                            .depth = texture.getImage().getDepth()
                        };
                        dst_context.getLogicalDevice()->vkCmdCopyBufferToImage(command_buffer,
                                                                               texture.getStagingBuffer().getBuffer(),
                                                                               texture.getVkImage(),
                                                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                               1, &copy_region);
                    }
                    state_machine.recordStateChange(&texture,
                                                    texture.getResourceState().clone()
                                                    .setPipelineStage(final_state.pipeline_stage)
                                                    .setAccessFlag(final_state.access_flag)
                                                    .setImageLayout(final_state.layout));
                    state_machine.commitChanges(command_buffer);
                };
            return upload_command;
        }

        std::function<void(VkCommandBuffer)> createTextureNotUnifiedUploadCommand(Texture& texture,
                                                                                  SingleShotCommandContext* src_context,
                                                                                  bool initial_transfer)
        {
            auto upload_command = [&texture, src_context, initial_transfer](VkCommandBuffer command_buffer)
                {
                    /*
                    * Image layout needs to be in dst optimal. When there is no queue ownership transformation
                    * (i.e.: it is an initial transfer) the layout transformation needs to be done here.
                    */
                    if (initial_transfer)
                    {
                        ResourceStateMachine state_machine(src_context->getLogicalDevice());
                        state_machine.recordStateChange(&texture,
                                                        texture.getResourceState().clone()
                                                        .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                        .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT)
                                                        .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                        );
                        state_machine.commitChanges(command_buffer);
                    }
                    {
                        VkBufferImageCopy copy_region{};
                        copy_region.bufferOffset = 0;
                        copy_region.bufferRowLength = 0;
                        copy_region.bufferImageHeight = 0;

                        copy_region.imageSubresource.aspectMask = texture.getAspect();
                        copy_region.imageSubresource.mipLevel = 0;
                        copy_region.imageSubresource.baseArrayLayer = 0;
                        copy_region.imageSubresource.layerCount = 1;

                        copy_region.imageOffset = { 0, 0, 0 };
                        copy_region.imageExtent = {
                            .width = texture.getImage().getWidth(),
                            .height = texture.getImage().getHeight(),
                            .depth = texture.getImage().getDepth()
                        };
                        src_context->getLogicalDevice()->vkCmdCopyBufferToImage(command_buffer,
                                                                                texture.getStagingBuffer().getBuffer(),
                                                                                texture.getVkImage(),
                                                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                                1, &copy_region);
                    }


                };
            return upload_command;
        }

        std::function<void(VkCommandBuffer)> createTextureNotUnifiedDownloadCommand(Texture& texture,
                                                                                    SingleShotCommandContext& src_context)
        {
            auto download_command = [&texture, &src_context](VkCommandBuffer command_buffer)
                {
                    VkBufferImageCopy copy_region{};
                    copy_region.bufferOffset = 0;
                    copy_region.bufferRowLength = 0;
                    copy_region.bufferImageHeight = 0;

                    copy_region.imageSubresource.aspectMask = texture.getAspect();
                    copy_region.imageSubresource.mipLevel = 0;
                    copy_region.imageSubresource.baseArrayLayer = 0;
                    copy_region.imageSubresource.layerCount = 1;

                    copy_region.imageOffset = { 0, 0, 0 };
                    copy_region.imageExtent = {
                        .width = texture.getImage().getWidth(),
                        .height = texture.getImage().getHeight(),
                        .depth = texture.getImage().getDepth()
                    };
                    src_context.getLogicalDevice()->vkCmdCopyImageToBuffer(command_buffer,
                                                                           texture.getVkImage(),
                                                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                           texture.getStagingBuffer().getBuffer(),
                                                                           1, &copy_region);

                };
            return download_command;
        }

        std::function<void(VkCommandBuffer)> createTextureUnifiedDownloadCommand(Texture& texture,
                                                                                 SingleShotCommandContext& src_context,
                                                                                 TextureState final_state)
        {
            auto download_command = [&](VkCommandBuffer command_buffer)
                {
                    ResourceStateMachine state_machine(src_context.getLogicalDevice());
                    auto old_state = texture.getResourceState();
                    state_machine.recordStateChange(&texture,
                                                    texture.getResourceState().clone()
                                                    .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                    .setAccessFlag(VK_ACCESS_2_TRANSFER_READ_BIT)
                                                    .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                    );
                    state_machine.commitChanges(command_buffer);
                    {
                        VkBufferImageCopy copy_region{};
                        copy_region.bufferOffset = 0;
                        copy_region.bufferRowLength = 0;
                        copy_region.bufferImageHeight = 0;

                        copy_region.imageSubresource.aspectMask = texture.getAspect();
                        copy_region.imageSubresource.mipLevel = 0;
                        copy_region.imageSubresource.baseArrayLayer = 0;
                        copy_region.imageSubresource.layerCount = 1;

                        copy_region.imageOffset = { 0, 0, 0 };
                        copy_region.imageExtent = {
                            .width = texture.getImage().getWidth(),
                            .height = texture.getImage().getHeight(),
                            .depth = texture.getImage().getDepth()
                        };
                        src_context.getLogicalDevice()->vkCmdCopyImageToBuffer(command_buffer,
                                                                               texture.getVkImage(),
                                                                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                               texture.getStagingBuffer().getBuffer(),
                                                                               1, &copy_region);
                    }

                    state_machine.recordStateChange(&texture, old_state);
                    state_machine.commitChanges(command_buffer);
                };
            return download_command;
        }
#pragma endregion

#pragma region Buffer Upload Commands
        std::function<void(VkCommandBuffer)> createBufferUnifiedUploadCommand(Buffer& buffer,
                                                                              VkBuffer staging_buffer,
                                                                              SingleShotCommandContext& src_context,
                                                                              BufferState final_buffer_state)
        {
            auto upload_command = [&buffer, &src_context, final_buffer_state, staging_buffer](VkCommandBuffer command_buffer)
                {
                    ResourceStateMachine state_machine(src_context.getLogicalDevice());
                    state_machine.recordStateChange(&buffer,
                                                    buffer.getResourceState().clone()
                                                    .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                    .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT));
                    state_machine.commitChanges(command_buffer);
                    VkBufferCopy copy_region{};
                    copy_region.size = buffer.getDeviceSize();
                    src_context.getLogicalDevice()->vkCmdCopyBuffer(command_buffer,
                                                                    staging_buffer,
                                                                    buffer.getBuffer(),
                                                                    1,
                                                                    &copy_region);
                    state_machine.recordStateChange(&buffer,
                                                    buffer.getResourceState().clone()
                                                    .setPipelineStage(final_buffer_state.pipeline_stage)
                                                    .setAccessFlag(final_buffer_state.access_flag));
                    state_machine.commitChanges(command_buffer);
                };
            return upload_command;
        }

        std::function<void(VkCommandBuffer)> createBufferNotUnifiedUploadCommand(Buffer& buffer,
                                                                                 VkBuffer staging_buffer,
                                                                                 SingleShotCommandContext& dst_context)
        {
            auto upload_command = [&buffer, staging_buffer, &dst_context](VkCommandBuffer command_buffer)
                {
                    VkBufferCopy copy_region{};
                    copy_region.size = buffer.getDeviceSize();
                    dst_context.getLogicalDevice()->vkCmdCopyBuffer(command_buffer,
                                                                    staging_buffer,
                                                                    buffer.getBuffer(),
                                                                    1,
                                                                    &copy_region);
                };
            return upload_command;
        }

#pragma endregion

#pragma region Queue Transfer Commands

        enum class DataTransferType
        {
            Upload,
            Download
        };
        [[nodiscard]]
        SyncObject unifiedQueueTransfer(
            SyncOperations sync_operations,
            TransferEngine& transfer_engine,
            std::function<void(VkCommandBuffer)> upload_command,
            QueueSubmitTracker* submit_tracker)
        {
            SyncObject transfer_sync_object(transfer_engine.getTransferContext().getLogicalDevice());
            transfer_sync_object.createTimelineSemaphore(DataTransferScheduler::kDataTransferFinishSemaphoreName, 0, 2);
            transfer_sync_object.addSignalOperationToGroup(SyncGroups::kInternal,
                                                           DataTransferScheduler::kDataTransferFinishSemaphoreName,
                                                           VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                           1);
            transfer_sync_object.addWaitOperationToGroup(SyncGroups::kExternal,
                                                         DataTransferScheduler::kDataTransferFinishSemaphoreName,
                                                         VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                         1);
            transfer_engine.transfer(sync_operations.createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
                                     upload_command,
                                     submit_tracker);
            return transfer_sync_object;
        }

        [[nodiscard]]
        std::vector<SyncObject> textureNotUnifiedQueueTransfer(Texture& texture,
                                                               SyncOperations sync_operations,
                                                               TransferEngine& transfer_engine,
                                                               SingleShotCommandContext& src_context,
                                                               SingleShotCommandContext& dst_context,
                                                               TextureState final_state,
                                                               std::function<void(VkCommandBuffer)> upload_command,
                                                               DataTransferType transfer_type,
                                                               QueueSubmitTracker* submit_tracker)
        {
            std::vector<SyncObject> result;
            const bool is_initial_transfer = texture.getResourceState().command_context.expired();


            SyncObject transfer_sync_object(src_context.getLogicalDevice());
            transfer_sync_object.createTimelineSemaphore(DataTransferScheduler::kDataTransferFinishSemaphoreName, 0, 2);
            transfer_sync_object.addSignalOperationToGroup(SyncGroups::kInternal,
                                                           DataTransferScheduler::kDataTransferFinishSemaphoreName,
                                                           VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                           1);
            transfer_sync_object.addWaitOperationToGroup(SyncGroups::kExternal,
                                                         DataTransferScheduler::kDataTransferFinishSemaphoreName,
                                                         VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                         1);
            if (is_initial_transfer == false)
            {
                VkImageLayout layout_for_copy = transfer_type == DataTransferType::Download
                    ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                    : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                SyncObject sync_object_src_to_transfer = ResourceStateMachine::transferOwnership(&texture,
                                                                                                 texture.getResourceState().clone()
                                                                                                 .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                                                                 .setAccessFlag(0)
                                                                                                 .setImageLayout(layout_for_copy),
                                                                                                 texture.getResourceState().command_context.lock().get(),
                                                                                                 &transfer_engine.getTransferContext(),
                                                                                                 sync_operations.extract(SyncOperations::ExtractWaitOperations),
                                                                                                 submit_tracker);

                transfer_engine.transfer(sync_object_src_to_transfer.getOperationsGroup(SyncGroups::kExternal)
                                         .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
                                         upload_command,
                                         submit_tracker);
                result.push_back(std::move(sync_object_src_to_transfer));

            }
            else
            {
                SyncObject execution_barrier = ResourceStateMachine::barrier(texture, src_context, sync_operations, submit_tracker);

                transfer_engine.transfer(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)
                                         .createUnionWith(execution_barrier.getOperationsGroup(SyncGroups::kExternal)),
                                         upload_command,
                                         submit_tracker);
                result.push_back(std::move(execution_barrier));

            }
            assert(texture.getResourceState().getQueueFamilyIndex() == transfer_engine.getTransferContext().getQueueFamilyIndex());

            SyncObject sync_object_transfer_to_dst = ResourceStateMachine::transferOwnership(&texture,
                                                                                             texture.getResourceState().clone()
                                                                                             .setPipelineStage(final_state.pipeline_stage)
                                                                                             .setAccessFlag(0)
                                                                                             .setImageLayout(final_state.layout),
                                                                                             texture.getResourceState().command_context.lock().get(),
                                                                                             &dst_context,
                                                                                             sync_operations.extract(SyncOperations::ExtractSignalOperations)
                                                                                             .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kExternal)),
                                                                                             submit_tracker);
            assert(texture.getResourceState().getQueueFamilyIndex() == dst_context.getQueueFamilyIndex());
            result.push_back(std::move(sync_object_transfer_to_dst));
            result.push_back(std::move(transfer_sync_object));
            return result;
        }

        [[nodiscard]]
        std::vector<SyncObject> bufferNotUnifiedQueueTransfer(Buffer& buffer,
                                                              SyncOperations sync_operations,
                                                              TransferEngine& transfer_engine,
                                                              SingleShotCommandContext& src_context,
                                                              SingleShotCommandContext& dst_context,
                                                              BufferState final_buffer_state,
                                                              std::function<void(VkCommandBuffer)> upload_command,
                                                              QueueSubmitTracker* submit_tracker)
        {
            std::vector<SyncObject> result;

            SyncObject transfer_sync_object(src_context.getLogicalDevice());
            transfer_sync_object.createTimelineSemaphore(DataTransferScheduler::kDataTransferFinishSemaphoreName, 0, 2);
            transfer_sync_object.addSignalOperationToGroup(SyncGroups::kInternal,
                                                           DataTransferScheduler::kDataTransferFinishSemaphoreName,
                                                           VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                           1);
            transfer_sync_object.addWaitOperationToGroup(SyncGroups::kExternal,
                                                         DataTransferScheduler::kDataTransferFinishSemaphoreName,
                                                         VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                         1);
            const bool is_initial_transfer = buffer.getResourceState().command_context.expired();

            if (is_initial_transfer == false)
            {
                SyncObject sync_object_src_to_transfer = ResourceStateMachine::transferOwnership(&buffer,
                                                                                                 buffer.getResourceState().clone()
                                                                                                 .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                                                                                                 .setAccessFlag(0),
                                                                                                 buffer.getResourceState().command_context.lock().get(),
                                                                                                 &transfer_engine.getTransferContext(),
                                                                                                 sync_operations.extract(SyncOperations::ExtractWaitOperations));
                transfer_engine.transfer(sync_object_src_to_transfer.getOperationsGroup(SyncGroups::kExternal)
                                         .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
                                         upload_command,
                                         submit_tracker);
                result.push_back(std::move(sync_object_src_to_transfer));

            }
            else
            {
                SyncObject execution_barrier = ResourceStateMachine::barrier(&buffer, &src_context, sync_operations);
                transfer_engine.transfer(execution_barrier.getOperationsGroup(SyncGroups::kExternal)
                                         .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
                                         upload_command,
                                         submit_tracker);
                result.push_back(std::move(execution_barrier));
            }
            assert(buffer.getResourceState().getQueueFamilyIndex() == transfer_engine.getTransferContext().getQueueFamilyIndex());

            SyncObject sync_object_transfer_to_dst = ResourceStateMachine::transferOwnership(&buffer,
                                                                                             buffer.getResourceState().clone()
                                                                                             .setPipelineStage(final_buffer_state.pipeline_stage)
                                                                                             .setAccessFlag(0),
                                                                                             buffer.getResourceState().command_context.lock().get(),
                                                                                             &dst_context,
                                                                                             sync_operations.extract(SyncOperations::ExtractSignalOperations)
                                                                                             .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kExternal))
            );
            assert(buffer.getResourceState().getQueueFamilyIndex() == dst_context.getQueueFamilyIndex());
            result.push_back(std::move(sync_object_transfer_to_dst));
            result.push_back(std::move(transfer_sync_object));
            return result;
        }
#pragma endregion
    } // namespace

    const std::string DataTransferScheduler::kDataTransferFinishSemaphoreName = "DataTransferFinished";


    DataTransferScheduler::~DataTransferScheduler() = default;

    std::weak_ptr<UploadTask> DataTransferScheduler::upload(Texture* texture,
                                                            Image image,
                                                            SingleShotCommandContext& dst_context,
                                                            TextureState final_state,
                                                            SyncOperations additional_sync_operations)
    {
        auto task = [image_to_upload = std::move(image), &dst_context, texture, final_texture_state = std::move(final_state), additional_sync_operations]
        (SyncOperations sync_operations, TransferEngine& transfer_engine, UploadTask::Storage&, QueueSubmitTracker& submit_tracker) -> std::vector<SyncObject>
            {
                if (texture->isImageCompatible(image_to_upload) == false)
                {
                    throw std::runtime_error("Input image is incompatible with the texture");
                }
                std::span<const uint8_t> data_view =
                    std::visit(overloaded{
                               [&](const std::vector<uint8_t>& image_data) { return std::span(image_data); },
                               [&](const std::vector<float>& image_data) { return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&image_data[0]), image_data.size() / sizeof(float)); } },
                               image_to_upload.getData());
                texture->getStagingBuffer().upload(data_view);
                const bool is_initial_transfer = texture->getResourceState().command_context.expired();

                if (is_initial_transfer)
                {
                    texture->setInitialCommandContext(transfer_engine.getTransferContext().getWeakReference());
                }

                auto* src_context = texture->getResourceState().command_context.lock().get();

                if (dst_context.getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex()
                    || src_context->getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex())
                {
                    return textureNotUnifiedQueueTransfer(*texture,
                                                          sync_operations.createUnionWith(additional_sync_operations),
                                                          transfer_engine,
                                                          *src_context,
                                                          dst_context,
                                                          std::move(final_texture_state),
                                                          createTextureNotUnifiedUploadCommand(*texture, src_context, is_initial_transfer),
                                                          DataTransferType::Upload,
                                                          &submit_tracker);
                }
                else
                {
                    SyncObject transfer_sync_object = unifiedQueueTransfer(sync_operations.createUnionWith(additional_sync_operations),
                                                                           transfer_engine,
                                                                           createTextureUnifiedUploadCommand(*texture, dst_context, std::move(final_texture_state)),
                                                                           &submit_tracker);
                    std::vector<SyncObject> result;
                    result.push_back(std::move(transfer_sync_object));
                    return result;
                }
            };
        std::shared_ptr<UploadTask> result = std::make_shared<UploadTask>(dst_context.getLogicalDevice(), std::move(task));
        _textures_staging_area.uploads[texture] = result;
        return result;
    }

    std::weak_ptr<UploadTask> DataTransferScheduler::upload(Buffer* buffer,
                                                            std::vector<uint8_t> data,
                                                            SingleShotCommandContext& dst_context,
                                                            BufferState final_state)
    {
        auto task = [data_to_upload = std::move(data), &dst_context, buffer, final_buffer_state = std::move(final_state)]
        (SyncOperations sync_operations, TransferEngine& transfer_engine, UploadTask::Storage& task_storage, QueueSubmitTracker& submit_tracker) -> std::vector<SyncObject>
            {
                auto& logical_device = buffer->getLogicalDevice();
                const auto device_size = buffer->getDeviceSize();

                if (device_size != data_to_upload.size())
                {
                    throw std::runtime_error("Invalid size during upload. Buffer size: " + std::to_string(device_size) + " data to upload: " + std::to_string(data_to_upload.size()));
                }
                auto [staging_buffer, staging_memory] = createBuffer(buffer->getPhysicalDevice(),
                                                                     logical_device,
                                                                     device_size,
                                                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                void* data = nullptr;
                logical_device->vkMapMemory(*logical_device, staging_memory, 0, device_size, 0, &data);
                memcpy(data, data_to_upload.data(), static_cast<size_t>(device_size));
                logical_device->vkUnmapMemory(*logical_device, staging_memory);

                const bool is_initial_transfer = buffer->getResourceState().command_context.expired();
                if (is_initial_transfer)
                {
                    buffer->setInitialCommandContext(transfer_engine.getTransferContext().getWeakReference());
                }
                auto* src_context = buffer->getResourceState().command_context.lock().get();
                std::vector<SyncObject> result;
                if (dst_context.getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex()
                    || src_context->getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex())
                {
                    result = bufferNotUnifiedQueueTransfer(*buffer,
                                                           sync_operations,
                                                           transfer_engine,
                                                           *src_context,
                                                           dst_context,
                                                           final_buffer_state,
                                                           createBufferNotUnifiedUploadCommand(*buffer,
                                                                                               staging_buffer,
                                                                                               dst_context),
                                                           &submit_tracker);
                }
                else
                {
                    SyncObject transfer_sync_object = unifiedQueueTransfer(sync_operations,
                                                                           transfer_engine,
                                                                           createBufferUnifiedUploadCommand(*buffer,
                                                                                                            staging_buffer,
                                                                                                            *src_context,
                                                                                                            final_buffer_state),
                                                                           &submit_tracker);
                    result.push_back(std::move(transfer_sync_object));
                }
                task_storage.storeStagingData(staging_buffer, staging_memory, &logical_device);
                return result;

            };
        std::shared_ptr<UploadTask> result = std::make_shared<UploadTask>(dst_context.getLogicalDevice(), std::move(task));
        _buffers_staging_area.uploads[buffer] = result;
        return result;
    }

    std::weak_ptr<DownloadTask> DataTransferScheduler::download(Texture* texture,
                                                                SyncOperations sync_operations)
    {
        assert(texture->getResourceState().command_context.expired() == false && "For download it should never be an initial transfer");

        auto task = [texture, additional_sync_operations = sync_operations]
        (SyncOperations sync_operations, TransferEngine& transfer_engine, QueueSubmitTracker& submit_tracker) -> std::vector<SyncObject>
            {
                auto src_context = texture->getResourceState().command_context.lock();
                if (src_context->getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex())
                {
                    return textureNotUnifiedQueueTransfer(*texture,
                                                          sync_operations.createUnionWith(additional_sync_operations),
                                                          transfer_engine,
                                                          *src_context,
                                                          *src_context,
                                                          texture->getResourceState().clone(),
                                                          createTextureNotUnifiedDownloadCommand(*texture, *src_context),
                                                          DataTransferType::Download,
                                                          &submit_tracker);
                }
                else
                {
                    SyncObject transfer_sync_object = unifiedQueueTransfer(sync_operations.createUnionWith(additional_sync_operations),
                                                                           transfer_engine,
                                                                           createTextureUnifiedDownloadCommand(*texture,
                                                                                                               *src_context,
                                                                                                               texture->getResourceState().clone()),
                                                                           &submit_tracker);
                    std::vector<SyncObject> result;
                    result.push_back(std::move(transfer_sync_object));
                    return result;
                }
            };
        std::shared_ptr<DownloadTask> result = std::make_shared<DownloadTask>(std::move(task), texture, texture->getResourceState().command_context.lock()->getLogicalDevice());
        _textures_staging_area.downloads[texture] = result;
        return result;
    }

    void DataTransferScheduler::executeTasks(SyncOperations sync_operations, TransferEngine& transfer_engine)
    {
        for (auto [texture, task] : _textures_staging_area.uploads)
        {
            texture->assignUploadTask(task);
            task->start({}, sync_operations, transfer_engine);
        }
        for (auto [texture, task] : _textures_staging_area.downloads)
        {
            texture->assignDownloadTask(task);
            task->start({}, sync_operations, transfer_engine);
        }
        for (auto [buffer, task] : _buffers_staging_area.uploads)
        {
            buffer->assignUploadTask(task);
            task->start({}, sync_operations, transfer_engine);
        }
        for (auto [buffer, task] : _buffers_staging_area.downloads)
        {
            buffer->assignDownloadTask(task);
            task->start({}, sync_operations, transfer_engine);
        }
        _textures_staging_area.downloads.clear();
        _textures_staging_area.uploads.clear();
        _buffers_staging_area.downloads.clear();
        _buffers_staging_area.uploads.clear();
    }
    std::weak_ptr<UploadTask> DataTransferScheduler::upload(Buffer* buffer, std::span<const uint8_t> data, SingleShotCommandContext& dst_context, BufferState final_state)
    {
        return upload(buffer,
                      std::vector(data.begin(), data.end()),
                      dst_context,
                      std::move(final_state));

    }
}
