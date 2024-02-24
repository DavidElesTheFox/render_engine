#include <render_engine/resources/Texture.h>

#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/RenderContext.h>

#include <cassert>
#include <span>

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

        // TODO fixed values for now
        constexpr uint32_t kMipLevel = 1;
        constexpr uint32_t kArrayLayers = 1;
        constexpr uint32_t kLayerCount = 1;
    }

    bool Texture::isImageCompatible(const Image& image) const
    {
        return image.getWidth() == _image.getWidth()
            && image.getHeight() == _image.getHeight()
            && image.getFormat() == _image.getFormat();
    }

    Texture::Texture(Image image,
                     VkPhysicalDevice physical_device,
                     LogicalDevice& logical_device,
                     VkImageAspectFlags aspect,
                     VkShaderStageFlags shader_usage,
                     std::set<uint32_t> compatible_queue_family_indexes,
                     VkImageUsageFlags image_usage,
                     bool support_external_usage)
        try : _physical_device(physical_device)
        , _logical_device(logical_device)
        , _staging_buffer(physical_device, logical_device, image.createBufferInfo())
        , _image(std::move(image))
        , _aspect(aspect)
        , _shader_usage(shader_usage)
        , _compatible_queue_family_indexes(std::move(compatible_queue_family_indexes))
    {
        std::vector<uint32_t> queue_family_indices{ _compatible_queue_family_indexes.begin(), _compatible_queue_family_indexes.end() };
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.format = _image.getFormat();
        image_info.mipLevels = kMipLevel;
        image_info.arrayLayers = kArrayLayers;

        image_info.extent.width = _image.getWidth();
        image_info.extent.height = _image.getHeight();
        image_info.extent.depth = _image.getDepth();

        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // TODO add initial layout for opimization
        image_info.imageType = image.is3D() ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT // for download
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT // for upload
            | image_usage;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.queueFamilyIndexCount = queue_family_indices.size();
        image_info.pQueueFamilyIndices = queue_family_indices.data();

        VkExternalMemoryImageCreateInfo external_create_info{};
        if (support_external_usage)
        {
            external_create_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
            external_create_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            image_info.pNext = &external_create_info;
        }

        _texture_state.layout = image_info.initialLayout;

        if (_logical_device->vkCreateImage(*_logical_device, &image_info, nullptr, &_texture) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create image");
        }

        _logical_device->vkGetImageMemoryRequirements(*_logical_device, _texture, &_memory_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = _memory_requirements.size;
        alloc_info.memoryTypeIndex = findMemoryType(_physical_device, _memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkExportMemoryAllocateInfo export_alloc_info{};

        if (support_external_usage)
        {
            export_alloc_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            export_alloc_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            alloc_info.pNext = &export_alloc_info;
        }
        if (_logical_device->vkAllocateMemory(*_logical_device, &alloc_info, nullptr, &_texture_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot allocate memory for image");
        }
        _logical_device->vkBindImageMemory(*_logical_device, _texture, _texture_memory, 0);
    }
    catch (const std::exception&)
    {
        destroy();
    }

    void Texture::setInitialCommandContext(std::weak_ptr<CommandContext> command_context)
    {
        if (_texture_state.command_context.expired() == false)
        {
            throw std::runtime_error("Texture has a command context which shouldn't be overwritten");
        }
        _texture_state.command_context = command_context;
    }

    std::shared_ptr<DownloadTask> Texture::clearDownloadTask()
    {
        auto result = _ongoing_download;
        _ongoing_download.reset();
        return result;
    }
    //std::vector<SyncObject> Texture::upload(const Image& image,
    //                                        SyncOperations sync_operations,
    //                                        TransferEngine& transfer_engine,
    //                                        CommandContext* dst_context)
    //{

    //    if (isImageCompatible(image) == false)
    //    {
    //        throw std::runtime_error("Input image is incompatible with the texture");
    //    }
    //    std::span<const uint8_t> data_view =
    //        std::visit(overloaded{
    //                   [&](const std::vector<uint8_t>& image_data) { return std::span(image_data); },
    //                   [&](const std::vector<float>& image_data) { return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&image_data[0]), image_data.size() / sizeof(float)); } },
    //                   image.getData());
    //    _staging_buffer.uploadMapped(data_view);
    //    const bool is_initial_transfer = _texture_state.command_context.expired();

    //    if (is_initial_transfer)
    //    {
    //        _texture_state.command_context = transfer_engine.getTransferContext().getWeakReference();
    //    }

    //    auto* src_context = _texture_state.command_context.lock().get();

    //    if (dst_context->getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex()
    //        || src_context->getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex())
    //    {
    //        return notUnifiedQueueTransfer(image, sync_operations, transfer_engine, src_context, dst_context);
    //    }
    //    else
    //    {
    //        return unifiedQueueTransfer(image, sync_operations, transfer_engine, dst_context);
    //    }
    //}

    //std::vector<SyncObject> Texture::unifiedQueueTransfer(const Image& image,
    //                                                      SyncOperations sync_operations,
    //                                                      TransferEngine& transfer_engine,
    //                                                      CommandContext* dst_context)
    //{
    //    auto upload_command = [&](VkCommandBuffer command_buffer)
    //        {
    //            ResourceStateMachine state_machine(dst_context->getLogicalDevice());
    //            state_machine.recordStateChange(this,
    //                                            getResourceState().clone()
    //                                            .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    //                                            .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT)
    //                                            .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    //            );
    //            state_machine.commitChanges(command_buffer);

    //            {
    //                VkBufferImageCopy copy_region{};
    //                copy_region.bufferOffset = 0;
    //                copy_region.bufferRowLength = 0;
    //                copy_region.bufferImageHeight = 0;

    //                copy_region.imageSubresource.aspectMask = _aspect;
    //                copy_region.imageSubresource.mipLevel = 0;
    //                copy_region.imageSubresource.baseArrayLayer = 0;
    //                copy_region.imageSubresource.layerCount = 1;

    //                copy_region.imageOffset = { 0, 0, 0 };
    //                copy_region.imageExtent = {
    //                    .width = _image.getWidth(),
    //                    .height = _image.getHeight(),
    //                    .depth = _image.getDepth()
    //                };
    //                transfer_engine.getTransferContext().getLogicalDevice()->vkCmdCopyBufferToImage(command_buffer,
    //                                                                                                _staging_buffer.getBuffer(),
    //                                                                                                _texture,
    //                                                                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //                                                                                                1, &copy_region);
    //            }

    //            auto getFinalStageMask = [&]
    //                {
    //                    VkPipelineStageFlagBits2 result{ 0 };
    //                    if (_shader_usage & VK_SHADER_STAGE_VERTEX_BIT)
    //                    {
    //                        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    //                    }
    //                    if (_shader_usage & VK_SHADER_STAGE_FRAGMENT_BIT)
    //                    {
    //                        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    //                    }
    //                    return result;
    //                };
    //            state_machine.recordStateChange(this,
    //                                            getResourceState().clone()
    //                                            .setPipelineStage(getFinalStageMask())
    //                                            .setAccessFlag(VK_ACCESS_2_SHADER_READ_BIT)
    //                                            .setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    //            state_machine.commitChanges(command_buffer);
    //        };
    //    transfer_engine.transfer(sync_operations, upload_command);
    //    return {};
    //}

    //std::vector<SyncObject> Texture::notUnifiedQueueTransfer(const Image& image,
    //                                                         SyncOperations sync_operations,
    //                                                         TransferEngine& transfer_engine,
    //                                                         CommandContext* src_context,
    //                                                         CommandContext* dst_context)
    //{
    //    std::vector<SyncObject> result;
    //    const bool is_initial_transfer = _texture_state.command_context.expired();

    //    auto upload_command = [&](VkCommandBuffer command_buffer)
    //        {
    //            ResourceStateMachine state_machine(src_context->getLogicalDevice());
    //            state_machine.recordStateChange(this,
    //                                            getResourceState().clone()
    //                                            .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    //                                            .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT)
    //                                            .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    //            );
    //            state_machine.commitChanges(command_buffer);
    //            {
    //                VkBufferImageCopy copy_region{};
    //                copy_region.bufferOffset = 0;
    //                copy_region.bufferRowLength = 0;
    //                copy_region.bufferImageHeight = 0;

    //                copy_region.imageSubresource.aspectMask = _aspect;
    //                copy_region.imageSubresource.mipLevel = 0;
    //                copy_region.imageSubresource.baseArrayLayer = 0;
    //                copy_region.imageSubresource.layerCount = 1;

    //                copy_region.imageOffset = { 0, 0, 0 };
    //                copy_region.imageExtent = {
    //                    .width = _image.getWidth(),
    //                    .height = _image.getHeight(),
    //                    .depth = _image.getDepth()
    //                };
    //                src_context->getLogicalDevice()->vkCmdCopyBufferToImage(command_buffer,
    //                                                                        _staging_buffer.getBuffer(),
    //                                                                        _texture,
    //                                                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //                                                                        1, &copy_region);
    //            }


    //        };
    //    SyncObject transfer_sync_object = SyncObject::CreateEmpty(src_context->getLogicalDevice());
    //    transfer_sync_object.createSemaphore("DataTransferFinished");
    //    transfer_sync_object.addSignalOperationToGroup(SyncGroups::kInternal, "DataTransferFinished", VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    //    transfer_sync_object.addWaitOperationToGroup(SyncGroups::kExternal, "DataTransferFinished", VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    //    if (is_initial_transfer == false)
    //    {
    //        SyncObject sync_object_src_to_transfer = ResourceStateMachine::transferOwnership(this,
    //                                                                                         getResourceState().clone()
    //                                                                                         .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    //                                                                                         .setAccessFlag(0)
    //                                                                                         .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
    //                                                                                         _texture_state.command_context.lock().get(),
    //                                                                                         &transfer_engine.getTransferContext(),
    //                                                                                         sync_operations.extract(SyncOperations::ExtractWaitOperations));

    //        transfer_engine.transfer(sync_object_src_to_transfer.getOperationsGroup(SyncGroups::kExternal)
    //                                 .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)),
    //                                 upload_command);
    //        result.push_back(std::move(sync_object_src_to_transfer));

    //    }
    //    else
    //    {
    //        SyncObject execution_barrier = ResourceStateMachine::barrier(this, src_context, sync_operations);

    //        transfer_engine.transfer(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)
    //                                 .createUnionWith(execution_barrier.getOperationsGroup(SyncGroups::kExternal)),
    //                                 upload_command);
    //        result.push_back(std::move(execution_barrier));

    //    }
    //    assert(getResourceState().getQueueFamilyIndex() == transfer_engine.getTransferContext().getQueueFamilyIndex());
    //    auto getFinalStageMask = [&]
    //        {
    //            VkPipelineStageFlagBits2 result{ 0 };
    //            if (_shader_usage & VK_SHADER_STAGE_VERTEX_BIT)
    //            {
    //                result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    //            }
    //            if (_shader_usage & VK_SHADER_STAGE_FRAGMENT_BIT)
    //            {
    //                result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    //            }
    //            return result;
    //        };
    //    SyncObject sync_object_transfer_to_dst = ResourceStateMachine::transferOwnership(this,
    //                                                                                     getResourceState().clone()
    //                                                                                     .setPipelineStage(getFinalStageMask())
    //                                                                                     .setAccessFlag(VK_ACCESS_2_SHADER_READ_BIT)
    //                                                                                     .setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
    //                                                                                     _texture_state.command_context.lock().get(),
    //                                                                                     dst_context,
    //                                                                                     sync_operations.extract(SyncOperations::ExtractSignalOperations | SyncOperations::ExtractFence)
    //                                                                                     .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kExternal)));
    //    assert(getResourceState().getQueueFamilyIndex() == dst_context->getQueueFamilyIndex());
    //    result.push_back(std::move(sync_object_transfer_to_dst));
    //    result.push_back(std::move(transfer_sync_object));
    //    return result;
    //}

    //std::vector<uint8_t> Texture::download(const SyncOperations& sync_operations,
    //                                       TransferEngine& transfer_engine,
    //                                       CommandContext* src_context)
    //{
    //    // TODO remove blocking download
    //    assert(sync_operations.hasAnyFence());
    //    std::vector<SyncObject> sync_objects_for_download;

    //    const bool is_initial_transfer = _texture_state.command_context.expired();

    //    if (is_initial_transfer)
    //    {
    //        _texture_state.command_context = transfer_engine.getTransferContext().getWeakReference();
    //    }

    //    auto download_command = [&](VkCommandBuffer command_buffer)
    //        {
    //            ResourceStateMachine state_machine(src_context->getLogicalDevice());
    //            auto old_state = getResourceState();
    //            state_machine.recordStateChange(this,
    //                                            getResourceState().clone()
    //                                            .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    //                                            .setAccessFlag(VK_ACCESS_2_TRANSFER_READ_BIT)
    //                                            .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    //            );
    //            state_machine.commitChanges(command_buffer);
    //            {
    //                VkBufferImageCopy copy_region{};
    //                copy_region.bufferOffset = 0;
    //                copy_region.bufferRowLength = 0;
    //                copy_region.bufferImageHeight = 0;

    //                copy_region.imageSubresource.aspectMask = _aspect;
    //                copy_region.imageSubresource.mipLevel = 0;
    //                copy_region.imageSubresource.baseArrayLayer = 0;
    //                copy_region.imageSubresource.layerCount = 1;

    //                copy_region.imageOffset = { 0, 0, 0 };
    //                copy_region.imageExtent = {
    //                    .width = _image.getWidth(),
    //                    .height = _image.getHeight(),
    //                    .depth = _image.getDepth()
    //                };
    //                src_context->getLogicalDevice()->vkCmdCopyImageToBuffer(command_buffer,
    //                                                                        _texture,
    //                                                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //                                                                        _staging_buffer.getBuffer(),
    //                                                                        1, &copy_region);
    //            }

    //            state_machine.recordStateChange(this, old_state);
    //            state_machine.commitChanges(command_buffer);
    //        };
    //    SyncObject transfer_sync_object = SyncObject::CreateEmpty(src_context->getLogicalDevice());
    //    transfer_sync_object.createSemaphore("DataTransferFinished");
    //    transfer_sync_object.addSignalOperationToGroup(SyncGroups::kInternal, "DataTransferFinished", VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    //    transfer_sync_object.addWaitOperationToGroup(SyncGroups::kExternal, "DataTransferFinished", VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    //    TextureState original_state = getResourceState().clone();
    //    if (src_context->getQueueFamilyIndex() != transfer_engine.getTransferContext().getQueueFamilyIndex())
    //    {
    //        if (is_initial_transfer == false)
    //        {
    //            SyncObject sync_object_src_to_transfer = ResourceStateMachine::transferOwnership(this,
    //                                                                                             getResourceState().clone()
    //                                                                                             .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    //                                                                                             .setAccessFlag(0)
    //                                                                                             .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
    //                                                                                             src_context,
    //                                                                                             &transfer_engine.getTransferContext(),
    //                                                                                             sync_operations.extract(SyncOperations::ExtractWaitOperations));

    //            transfer_engine.transfer(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)
    //                                     .createUnionWith(sync_object_src_to_transfer.getOperationsGroup(SyncGroups::kExternal)),
    //                                     download_command);
    //            sync_objects_for_download.push_back(std::move(sync_object_src_to_transfer));

    //        }
    //        else
    //        {
    //            SyncObject execution_barrier = ResourceStateMachine::barrier(this,
    //                                                                         src_context,
    //                                                                         sync_operations.extract(SyncOperations::ExtractWaitOperations));
    //            transfer_engine.transfer(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)
    //                                     .createUnionWith(execution_barrier.getOperationsGroup(SyncGroups::kExternal)),
    //                                     download_command);
    //            sync_objects_for_download.push_back(std::move(execution_barrier));
    //        }

    //        SyncObject sync_object_transfer_to_dst = ResourceStateMachine::transferOwnership(this,
    //                                                                                         getResourceState().clone()
    //                                                                                         .setPipelineStage(original_state.pipeline_stage)
    //                                                                                         .setAccessFlag(original_state.access_flag)
    //                                                                                         .setImageLayout(original_state.layout),
    //                                                                                         &transfer_engine.getTransferContext(),
    //                                                                                         src_context,
    //                                                                                         sync_operations.extract(SyncOperations::ExtractSignalOperations | SyncOperations::ExtractFence)
    //                                                                                         .createUnionWith(transfer_sync_object.getOperationsGroup(SyncGroups::kExternal)));
    //        assert(getResourceState().getQueueFamilyIndex() == src_context->getQueueFamilyIndex());
    //        sync_objects_for_download.push_back(std::move(sync_object_transfer_to_dst));
    //        sync_objects_for_download.push_back(std::move(transfer_sync_object));
    //    }
    //    else
    //    {
    //        transfer_engine.transfer(transfer_sync_object.getOperationsGroup(SyncGroups::kInternal)
    //                                 .createUnionWith(sync_operations),
    //                                 download_command);

    //    }

    //    _logical_device->vkWaitForFences(*_logical_device, 1, sync_operations.getFence(), true, UINT64_MAX);
    //    _logical_device->vkResetFences(*_logical_device, 1, sync_operations.getFence());

    //    std::vector<uint8_t> data(_staging_buffer.getDeviceSize());
    //    const uint8_t* staging_memory = static_cast<const uint8_t*>(_staging_buffer.getMemory());
    //    std::copy(staging_memory,
    //              staging_memory + data.size(), data.data());
    //    return data;
    //}

    VkImageSubresourceRange Texture::createSubresourceRange() const
    {
        VkImageSubresourceRange result{};
        result.aspectMask = _aspect;
        result.baseMipLevel = 0;
        result.baseArrayLayer = 0;
        result.layerCount = kLayerCount;
        result.levelCount = 1;
        return result;
    }

    VkImageView Texture::createImageView(const ImageViewData& data)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = _texture;
        create_info.format = _image.getFormat();
        create_info.viewType = _image.is3D() ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;
        create_info.subresourceRange.aspectMask = _aspect;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;

        VkImageView result;
        if (_logical_device->vkCreateImageView(*_logical_device, &create_info, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create image view");
        }
        return result;
    }

    VkSampler Texture::createSampler(const SamplerData& data)
    {
        VkSamplerCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        create_info.addressModeU = data.sampler_address_mode;
        create_info.addressModeV = data.sampler_address_mode;
        create_info.addressModeW = data.sampler_address_mode;

        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(_physical_device, &properties);
            create_info.anisotropyEnable = data.anisotropy_filter_enabled;
            create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        }
        create_info.borderColor = data.border_color;

        create_info.compareEnable = false;

        create_info.magFilter = data.mag_filter;
        create_info.minFilter = data.min_filter;

        create_info.unnormalizedCoordinates = data.unnormalize_coordinate;

        create_info.minLod = 0;
        create_info.maxLod = 0;
        create_info.mipLodBias = 0;
        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        VkSampler result;
        if (_logical_device->vkCreateSampler(*_logical_device, &create_info, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create sampler");
        }

        return result;
    }

    void Texture::destroy() noexcept
    {
        _logical_device->vkFreeMemory(*_logical_device, _texture_memory, nullptr);
        _logical_device->vkDestroyImage(*_logical_device, _texture, nullptr);
    }

    HANDLE Texture::getMemoryHandle() const
    {
        HANDLE result = nullptr;

        VkMemoryGetWin32HandleInfoKHR memory_handle_info{};
        memory_handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        memory_handle_info.memory = _texture_memory;
        memory_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        assert(_logical_device->vkGetMemoryWin32HandleKHR != nullptr);
        _logical_device->vkGetMemoryWin32HandleKHR(*_logical_device, &memory_handle_info, &result);
        return result;
    }

    void Texture::assignUploadTask(std::shared_ptr<UploadTask> task)
    {
        if (_ongoing_upload != nullptr && _ongoing_upload->isFinished() == false)
        {
            throw std::runtime_error("Texture upload is still ongoing. New task shouldn't be started");
        }
        _ongoing_upload = task;
    }

    void Texture::assignDownloadTask(std::shared_ptr<DownloadTask> task)
    {
        if (_ongoing_download != nullptr)
        {
            throw std::runtime_error("Texture download is still ongoing. New task shouldn't be started");
        }
        _ongoing_download = task;
    }


    std::unique_ptr<TextureViewReference> TextureView::createReference() const
    {
        return std::unique_ptr<TextureViewReference>(new TextureViewReference{ TextureView(*this) });
    }

    std::unique_ptr<ITextureView> TextureView::clone() const
    {
        return createReference();
    }

    [[nodiscar]]
    std::unique_ptr<Texture> TextureFactory::create(Image image,
                                                    VkImageAspectFlags aspect,
                                                    VkShaderStageFlags shader_usage,
                                                    const SyncOperations& sync_operations,
                                                    CommandContext* dst_context,
                                                    VkImageUsageFlagBits image_usage,
                                                    TextureState final_state)
    {
        constexpr bool support_external_usage = false;
        std::unique_ptr<Texture> result{ new Texture(image,
            _physical_device, _logical_device,
            aspect,
            shader_usage,
            _compatible_queue_family_indexes,
            image_usage,
            support_external_usage) };
        _data_transfer_scheduler.upload(result.get(),
                                        std::move(image),
                                        *dst_context,
                                        final_state,
                                        sync_operations);
        return result;
    }
    [[nodiscar]]
    std::unique_ptr<Texture> TextureFactory::createExternal(Image image,
                                                            VkImageAspectFlags aspect,
                                                            VkShaderStageFlags shader_usage,
                                                            const SyncOperations& sync_operations,
                                                            CommandContext* dst_context,
                                                            VkImageUsageFlagBits image_usage,
                                                            TextureState final_state)
    {
        constexpr bool support_external_usage = true;
        std::unique_ptr<Texture> result{ new Texture(image,
            _physical_device, _logical_device,
            aspect,
            shader_usage,
            _compatible_queue_family_indexes,
            image_usage,
            support_external_usage) };
        _data_transfer_scheduler.upload(result.get(),
                                        std::move(image),
                                        *dst_context,
                                        final_state,
                                        sync_operations);
        return result;
    }

    std::unique_ptr<Texture> TextureFactory::createNoUpload(Image image,
                                                            VkImageAspectFlags aspect,
                                                            VkShaderStageFlags shader_usage,
                                                            VkImageUsageFlags image_usage)
    {
        constexpr bool support_external_usage = false;

        std::unique_ptr<Texture> result{ new Texture(image,
            _physical_device, _logical_device,
            aspect,
            shader_usage,
            _compatible_queue_family_indexes,
            image_usage,
            support_external_usage) };
        return result;
    }

    std::unique_ptr<Texture> TextureFactory::createExternalNoUpload(Image image,
                                                                    VkImageAspectFlags aspect,
                                                                    VkShaderStageFlags shader_usage,
                                                                    VkImageUsageFlags image_usage)
    {
        constexpr bool support_external_usage = true;

        std::unique_ptr<Texture> result{ new Texture(image,
            _physical_device, _logical_device,
            aspect,
            shader_usage,
            _compatible_queue_family_indexes,
            image_usage,
            support_external_usage) };
        return result;
    }
}