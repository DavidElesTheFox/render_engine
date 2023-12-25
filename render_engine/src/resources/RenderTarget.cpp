#include <render_engine/resources/RenderTarget.h>

#include <render_engine/assets/Image.h>
#include <render_engine/window/SwapChain.h>

namespace RenderEngine
{

    RenderTarget::RenderTarget(std::vector<VkImageView> images,
                               uint32_t width,
                               uint32_t height,
                               VkFormat image_format,
                               VkImageLayout layout)
        : _image_views(std::move(images))
        , _extent{ .width = width, .height = height }
        , _image_format(image_format)
        , _layout(layout)
    {}
    Image RenderTarget::createImage() const
    {
        return Image(_extent.width, _extent.height, _image_format);
    }
}