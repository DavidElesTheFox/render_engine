#pragma once

#include <volk.h>

#include <memory>
#include <vector>
namespace RenderEngine
{
    class SwapChain;
    class Texture;
    class Image;
    class RenderTarget
    {
    public:
        RenderTarget(std::vector<VkImageView> images,
                     uint32_t width,
                     uint32_t height,
                     VkFormat image_format,
                     VkImageLayout layout);
        ~RenderTarget() = default;
        uint32_t getWidth() const { return _extent.width; }
        uint32_t getHeight() const { return _extent.height; }
        VkFormat getImageFormat() const { return _image_format; }
        VkImageLayout getLayout() const { return _layout; }
        const std::vector<VkImageView>& getImageViews() const { return _image_views; }
        VkExtent2D getExtent() const { return _extent; }
        Image createImage() const;
    private:

        std::vector<VkImageView> _image_views;
        VkExtent2D _extent{};
        VkFormat _image_format{ VK_FORMAT_UNDEFINED };
        VkImageLayout _layout{ VK_IMAGE_LAYOUT_UNDEFINED };
    };
}