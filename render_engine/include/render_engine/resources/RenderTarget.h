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
                     VkImageLayout initial_layout,
                     VkImageLayout final_layout,
                     VkAttachmentLoadOp load_operation,
                     VkAttachmentStoreOp store_operation);
        ~RenderTarget();
        uint32_t getWidth() const { return _extent.width; }
        uint32_t getHeight() const { return _extent.height; }
        VkFormat getImageFormat() const { return _image_format; }
        VkImageLayout getInitialLayout() const { return _initial_layout; }
        VkImageLayout getFinalLayout() const { return _final_layout; }
        VkAttachmentLoadOp getLoadOperation() const { return _load_operation; }
        VkAttachmentStoreOp getStoreOperation() const { return _store_operation; }
        const ITextureView* getTextureView(uint32_t index) const { return _texture_views[index]; }
        ITextureView* getTextureView(uint32_t index) { return _texture_views[index]; }
        VkExtent2D getExtent() const { return _extent; }
        const Image& getImage(uint32_t index) const;
        uint32_t getTexturesCount() const { return static_cast<uint32_t>(_texture_views.size()); }

        RenderTarget clone() const { return *this; }
        RenderTarget&& changeInitialLayout(VkImageLayout value)&&
        {
            _initial_layout = value;
            return std::move(*this);
        }
        RenderTarget&& changeFinalLayout(VkImageLayout value)&&
        {
            _final_layout = value;
            return std::move(*this);
        }
        RenderTarget&& changeLoadOperation(VkAttachmentLoadOp value)&&
        {
            _load_operation = value;
            return std::move(*this);
        }
        RenderTarget&& changeStoreOperation(VkAttachmentStoreOp value)&&
        {
            _store_operation = value;
            return std::move(*this);
        }
    private:

        std::vector<ITextureView*> _texture_views;
        VkExtent2D _extent{};
        VkFormat _image_format{ VK_FORMAT_UNDEFINED };
        VkImageLayout _initial_layout{ VK_IMAGE_LAYOUT_UNDEFINED };
        VkImageLayout _final_layout{ VK_IMAGE_LAYOUT_UNDEFINED };
        VkAttachmentLoadOp _load_operation{ VK_ATTACHMENT_LOAD_OP_DONT_CARE };
        VkAttachmentStoreOp _store_operation{ VK_ATTACHMENT_STORE_OP_NONE };
    };
}