#include <render_engine/resources/RenderTarget.h>

#include <render_engine/assets/Image.h>
#include <render_engine/resources/Texture.h>
#include <render_engine/window/SwapChain.h>

namespace RenderEngine
{

    RenderTarget::RenderTarget(std::vector<ITextureView*> texture_views,
                               uint32_t width,
                               uint32_t height,
                               VkFormat image_format,
                               VkImageLayout layout)
        : _texture_views(std::move(texture_views))
        , _extent{ .width = width, .height = height }
        , _image_format(image_format)
        , _layout(layout)
    {}
    const Image& RenderTarget::getImage(uint32_t index) const
    {
        return _texture_views[index]->getTexture().getImage();
    }
    RenderTarget::~RenderTarget() = default;

}