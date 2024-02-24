#pragma once

#include <volk.h>

#include <memory>
#include <vector>
namespace RenderEngine
{
    class SwapChain;
    class ITextureView;
    class Image;
    class RenderTarget
    {
    public:
        RenderTarget(std::vector<ITextureView*> texture_views,
                     uint32_t width,
                     uint32_t height,
                     VkFormat image_format,
                     VkImageLayout layout);
        ~RenderTarget();
        uint32_t getWidth() const { return _extent.width; }
        uint32_t getHeight() const { return _extent.height; }
        VkFormat getImageFormat() const { return _image_format; }
        VkImageLayout getLayout() const { return _layout; }
        const ITextureView& getTextureView(uint32_t index) const { return *_texture_views[index]; }
        ITextureView& getTextureView(uint32_t index) { return *_texture_views[index]; }
        VkExtent2D getExtent() const { return _extent; }
        const Image& getImage(uint32_t index) const;
        uint32_t getTexturesCount() const { return static_cast<uint32_t>(_texture_views.size()); }
    private:

        std::vector<ITextureView*> _texture_views;
        VkExtent2D _extent{};
        VkFormat _image_format{ VK_FORMAT_UNDEFINED };
        VkImageLayout _layout{ VK_IMAGE_LAYOUT_UNDEFINED };
    };
}