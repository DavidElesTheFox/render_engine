#pragma once

#include <volk.h>

#include <render_engine/assets/Image.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/ResourceStateMachine.h>
#include <render_engine/SynchronizationPrimitives.h>
#include <render_engine/TransferEngine.h>

#include <set>

namespace RenderEngine
{
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

        /*
         * TODO: Textures needs to have only constant interface.These functions should be removed
         *       It is because of the synchronization. Textures are owned by the application layer.
         *       If they can be changed any time any where it makes impossible to synchronize the operation
         *       with the renderer.
         *       Probably the best solution if textures has a readonly interface. When a texture needs to be changed
         *       it needs to be done via a material instance. Material instance is the connection between the asset-renderer world.
         *       So, it makes sense to add there queued command to change textures and keeps the synchronization with the corresponding renders.
         */
        [[nodiscard]]
        TransferEngine::InFlightData upload(const Image& image,
                                            const SyncOperations& sync_operations,
                                            TransferEngine& transfer_engine,
                                            uint32_t dst_queue_family_index);

        [[nodiscard]]
        std::vector<uint8_t> download(const SyncOperations& sync_operations,
                                      TransferEngine& transfer_engine);

        VkImageView createImageView(const ImageViewData& data);
        VkSampler createSampler(const SamplerData& data);

        VkImageSubresourceRange createSubresourceRange() const;
        VkImage getVkImage() const { return _texture; }
        const Image& getImage() const { return _image; }

        const ResourceStateMachine::TextureState& getResourceState() const
        {
            return _texture_state;
        }

        HANDLE getMemoryHandle() const;
    private:
        Texture(Image image,
                VkPhysicalDevice physical_device,
                VkDevice logical_device,
                VkImageAspectFlags aspect,
                VkShaderStageFlags shader_usage,
                std::set<uint32_t> compatible_queue_family_indexes,
                VkImageUsageFlags image_usage,
                bool support_external_usage);
        void destroy() noexcept;

        void overrideResourceState(ResourceStateMachine::TextureState value)
        {
            _texture_state = std::move(value);
        }
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        VkDevice _logical_device{ VK_NULL_HANDLE };
        VkImage _texture{ VK_NULL_HANDLE };
        Image _image;
        VkDeviceMemory _texture_memory{ VK_NULL_HANDLE };
        Buffer _staging_buffer;
        VkImageAspectFlags _aspect{ VK_IMAGE_ASPECT_NONE };
        VkShaderStageFlags _shader_usage{ VK_SHADER_STAGE_ALL };
        std::set<uint32_t> _compatible_queue_family_indexes;
        ResourceStateMachine::TextureState _texture_state;
    };

    class ITextureView
    {
    public:
        virtual ~ITextureView() = default;
        virtual Texture& getTexture() = 0;
        virtual VkImageView getImageView() = 0;
        virtual VkSampler getSamler() = 0;
        virtual std::unique_ptr<ITextureView> clone() const = 0;
    };
    class TextureView : public ITextureView
    {
    public:
        friend class TextureViewReference;

        TextureView(Texture& texture,
                    Texture::ImageViewData image_view_data,
                    std::optional<Texture::SamplerData> sampler_data,
                    VkPhysicalDevice physical_device,
                    VkDevice logical_device)
            : _texture(texture)
            , _image_view(texture.createImageView(image_view_data))
            , _sampler(sampler_data != std::nullopt ? texture.createSampler(*sampler_data) : VK_NULL_HANDLE)
            , _logical_device(logical_device)
            , _image_view_data(std::move(image_view_data))
            , _sampler_data(std::move(sampler_data))
        {}
        ~TextureView() override
        {
            if (_logical_device)
            {
                vkDestroyImageView(_logical_device, _image_view, nullptr);
                vkDestroySampler(_logical_device, _sampler, nullptr);
            }
        }
        TextureView(TextureView&& o)
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
        VkImageView getImageView() override final { return _image_view; }
        VkSampler getSamler() override final { return _sampler; }

        std::unique_ptr<TextureViewReference> createReference();

        std::unique_ptr<ITextureView> clone() const override final
        {
            return std::unique_ptr<TextureView>(new TextureView(_texture,
                                                                _image_view_data,
                                                                _sampler_data,
                                                                _physical_device,
                                                                _logical_device));
        }
    protected:
        /**
        * Used when TextureViewReference is created
        */
        TextureView(const TextureView& o) = default;

        void disableObjectDestroy() noexcept
        {
            _logical_device = VK_NULL_HANDLE;
            _physical_device = VK_NULL_HANDLE;
        }
    private:

        Texture& _texture;
        VkImageView _image_view{ VK_NULL_HANDLE };
        VkSampler _sampler{ VK_NULL_HANDLE };
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        VkDevice _logical_device{ VK_NULL_HANDLE };
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
        VkImageView getImageView() override final { return _texture_view.getImageView(); }
        VkSampler getSamler() override final { return _texture_view.getSamler(); }
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
                       std::set<uint32_t> compatible_queue_family_indexes,
                       VkPhysicalDevice physical_device,
                       VkDevice logical_device)
            : _transfer_engine(transfer_engine)
            , _compatible_queue_family_indexes(std::move(compatible_queue_family_indexes))
            , _physical_device(physical_device)
            , _logical_device(logical_device)
        {}

        [[nodiscar]]
        std::tuple<std::unique_ptr<Texture>, TransferEngine::InFlightData> create(Image image,
                                                                                  VkImageAspectFlags aspect,
                                                                                  VkShaderStageFlags shader_usage,
                                                                                  const SyncOperations& synchronization_primitive,
                                                                                  uint32_t dst_queue_family_index,
                                                                                  VkImageUsageFlagBits image_usage);

        [[nodiscar]]
        std::unique_ptr<Texture> createNoUpload(Image image,
                                                VkImageAspectFlags aspect,
                                                VkShaderStageFlags shader_usage,
                                                VkImageUsageFlags image_usage);

        [[nodiscar]]
        std::unique_ptr<Texture> createExternalNoUpload(Image image,
                                                        VkImageAspectFlags aspect,
                                                        VkShaderStageFlags shader_usage,
                                                        VkImageUsageFlags image_usage);
    private:

        TransferEngine& _transfer_engine;
        std::set<uint32_t> _compatible_queue_family_indexes;
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        VkDevice _logical_device{ VK_NULL_HANDLE };
    };
}