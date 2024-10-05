#pragma once

#include <volk.h>

#include <render_engine/assets/Image.h>
#include <render_engine/DataTransferScheduler.h>
#include <render_engine/DataTransferTasks.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/synchronization/ResourceStateMachine.h>
#include <render_engine/synchronization/SyncOperations.h>
#include <render_engine/TransferEngine.h>

#include <set>

namespace RenderEngine
{
    class TextureView;

    class Texture
    {
    public:
        friend class ResourceStateMachine;
        friend class TextureFactory;
        struct ImageViewData
        {
        };

        struct SamplerData
        {
            VkFilter mag_filter{ VK_FILTER_LINEAR };
            VkFilter min_filter{ VK_FILTER_LINEAR };
            VkSamplerAddressMode sampler_address_mode{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
            bool anisotropy_filter_enabled{ false };
            VkBorderColor border_color{ VK_BORDER_COLOR_INT_OPAQUE_BLACK };
            bool unnormalize_coordinate{ false };
        };

        ~Texture()
        {
            destroy();
        }
        bool isImageCompatible(const Image& image) const;

        std::unique_ptr<TextureView> createTextureView(ImageViewData view_data,
                                                       std::optional<Texture::SamplerData> sampler_data);

        VkImageView createImageView(const ImageViewData& data);
        VkSampler createSampler(const SamplerData& data);

        VkImageSubresourceRange createSubresourceRange() const;
        VkImage getVkImage() const { return _texture; }
        const Image& getImage() const { return _image; }

        const TextureState& getResourceState() const
        {
            return _texture_state;
        }

        HANDLE getMemoryHandle() const;
        const VkMemoryRequirements& getMemoryRequirements() const { return _memory_requirements; }
        void overrideResourceState(TextureState value, ResourceAccessToken)
        {
            _texture_state = std::move(value);
        }
        void assignUploadTask(std::shared_ptr<UploadTask>);
        void assignDownloadTask(std::shared_ptr<DownloadTask>);


        std::shared_ptr<DownloadTask> clearDownloadTask();
        std::shared_ptr<UploadTask> getUploadTask() { return _ongoing_upload; }

        VkImageAspectFlags getAspect() const { return _aspect; }
        CoherentBuffer& getStagingBuffer() { return _staging_buffer; }
        VkShaderStageFlags getShaderUsageFlag() const { return _shader_usage; }

        void setInitialCommandContext(std::weak_ptr<SingleShotCommandBufferFactory> command_context);
    private:
        Texture(Image image,
                VkPhysicalDevice physical_device,
                LogicalDevice& logical_device,
                VkImageAspectFlags aspect,
                VkShaderStageFlags shader_usage,
                std::set<uint32_t> compatible_queue_family_indexes,
                VkImageUsageFlags image_usage,
                bool support_external_usage);
        Texture(Image image,
                VkImage texture,
                VkPhysicalDevice physical_device,
                LogicalDevice& logical_device,
                VkImageAspectFlags aspect);
        void destroy() noexcept;

        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice& _logical_device;
        VkImage _texture{ VK_NULL_HANDLE };
        CoherentBuffer _staging_buffer;
        Image _image;
        VkImageAspectFlags _aspect{ VK_IMAGE_ASPECT_NONE };
        VkShaderStageFlags _shader_usage{ VK_SHADER_STAGE_ALL };
        std::set<uint32_t> _compatible_queue_family_indexes;
        bool _vkimage_owner{ true };

        VkDeviceMemory _texture_memory{ VK_NULL_HANDLE };
        TextureState _texture_state;
        VkMemoryRequirements _memory_requirements{};
        std::shared_ptr<UploadTask> _ongoing_upload{ nullptr };
        std::shared_ptr<DownloadTask> _ongoing_download{ nullptr };
    };

    static_assert(IsResourceStateHolder_V<Texture>, "Texture needs to be a resource state holder");

    class ITextureView
    {
    public:
        virtual ~ITextureView() = default;
        virtual Texture& getTexture() = 0;
        virtual VkImageView getImageView() const = 0;
        virtual VkSampler getSampler() const = 0;
        virtual std::unique_ptr<ITextureView> clone() const = 0;
    };
    class TextureView : public ITextureView
    {
    public:
        friend class Texture;
        friend class TextureViewReference;

        ~TextureView() override
        {
            if (_logical_device == nullptr)
            {
                return;
            }
            auto& logical_device = *_logical_device;
            logical_device->vkDestroyImageView(*logical_device, _image_view, nullptr);
            logical_device->vkDestroySampler(*logical_device, _sampler, nullptr);
        }
        TextureView(TextureView&& o) noexcept
            : _texture(o._texture)
            , _image_view(std::move(o._image_view))
            , _sampler(std::move(o._sampler))
            , _physical_device(std::move(o._physical_device))
            , _logical_device(std::move(o._logical_device))
        {
            o._image_view = VK_NULL_HANDLE;
            o._sampler = VK_NULL_HANDLE;
            o._physical_device = VK_NULL_HANDLE;
            o._logical_device = VK_NULL_HANDLE;
        }
        TextureView& operator=(TextureView&&) = delete;

        TextureView& operator=(const TextureView&) = delete;

        Texture& getTexture() override final { return _texture; }
        VkImageView getImageView() const override final { return _image_view; }
        VkSampler getSampler() const override final { return _sampler; }

        std::unique_ptr<TextureViewReference> createReference() const;

        std::unique_ptr<ITextureView> clone() const override final;
    private:
        TextureView(Texture& texture,
                    Texture::ImageViewData image_view_data,
                    std::optional<Texture::SamplerData> sampler_data,
                    LogicalDevice& logical_device)
            : _texture(texture)
            , _image_view(texture.createImageView(image_view_data))
            , _sampler(sampler_data != std::nullopt ? texture.createSampler(*sampler_data) : VK_NULL_HANDLE)
            , _logical_device(&logical_device)
            , _image_view_data(std::move(image_view_data))
            , _sampler_data(std::move(sampler_data))
        {}

        /**
        * Used when TextureViewReference is created
        */
        TextureView(const TextureView& o) = default;

        void disableObjectDestroy() noexcept
        {
            _logical_device = nullptr;
            _physical_device = VK_NULL_HANDLE;
        }

        Texture& _texture;
        VkImageView _image_view{ VK_NULL_HANDLE };
        VkSampler _sampler{ VK_NULL_HANDLE };
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice* _logical_device{ nullptr };
        Texture::ImageViewData _image_view_data;
        std::optional<Texture::SamplerData> _sampler_data{ std::nullopt };
    };

    class TextureViewReference : public ITextureView
    {
    public:
        friend class TextureView;
        ~TextureViewReference() override
        {
            _texture_view.disableObjectDestroy();
        }
        TextureViewReference(TextureViewReference&&) = default;
        TextureViewReference& operator=(TextureViewReference&&) = default;

        TextureViewReference(const TextureViewReference& o) = delete;
        TextureViewReference& operator=(const TextureViewReference&) = delete;

        Texture& getTexture() override final { return _texture_view.getTexture(); }
        VkImageView getImageView() const override final { return _texture_view.getImageView(); }
        VkSampler getSampler() const override final { return _texture_view.getSampler(); }
        std::unique_ptr<ITextureView> clone() const override final
        {
            return std::unique_ptr<TextureViewReference>(new TextureViewReference(_texture_view));
        }
    private:
        TextureViewReference(TextureView texture_view)
            : _texture_view(std::move(texture_view))
        {}
        TextureView _texture_view;
    };

    class TextureFactory
    {
    public:
        TextureFactory(TransferEngine& transfer_engine,
                       DataTransferScheduler& data_transfer_scheduler,
                       std::set<uint32_t> compatible_queue_family_indexes,
                       VkPhysicalDevice physical_device,
                       LogicalDevice& logical_device)
            : _transfer_engine(transfer_engine)
            , _data_transfer_scheduler(data_transfer_scheduler)
            , _compatible_queue_family_indexes(std::move(compatible_queue_family_indexes))
            , _physical_device(physical_device)
            , _logical_device(logical_device)
        {}
        TextureFactory(const TextureFactory&) = delete;
        TextureFactory(TextureFactory&&) = delete;

        TextureFactory& operator=(const TextureFactory&) = delete;
        TextureFactory& operator=(TextureFactory&&) = delete;

        [[nodiscard]]
        std::unique_ptr<Texture> create(Image image,
                                        VkImageAspectFlags aspect,
                                        VkShaderStageFlags shader_usage,
                                        const SyncOperations& synchronization_primitive,
                                        std::shared_ptr<SingleShotCommandBufferFactory> dst_context,
                                        VkImageUsageFlagBits image_usage,
                                        TextureState final_state);
        [[nodiscard]]
        std::unique_ptr<Texture> createExternal(Image image,
                                                VkImageAspectFlags aspect,
                                                VkShaderStageFlags shader_usage,
                                                const SyncOperations& synchronization_primitive,
                                                std::shared_ptr<SingleShotCommandBufferFactory> dst_context,
                                                VkImageUsageFlagBits image_usage,
                                                TextureState final_state);
        [[nodiscard]]
        std::unique_ptr<Texture> createNoUpload(Image image,
                                                VkImageAspectFlags aspect,
                                                VkShaderStageFlags shader_usage,
                                                VkImageUsageFlags image_usage);

        [[nodiscard]]
        std::unique_ptr<Texture> createExternalNoUpload(Image image,
                                                        VkImageAspectFlags aspect,
                                                        VkShaderStageFlags shader_usage,
                                                        VkImageUsageFlags image_usage);

        [[nodiscard]]
        std::unique_ptr<Texture> createWrapper(Image image,
                                               VkImage texture,
                                               VkPhysicalDevice physical_device,
                                               LogicalDevice& logical_device,
                                               VkImageAspectFlags aspect);
    private:

        TransferEngine _transfer_engine;
        DataTransferScheduler& _data_transfer_scheduler;
        std::set<uint32_t> _compatible_queue_family_indexes;
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        LogicalDevice& _logical_device;
    };

}