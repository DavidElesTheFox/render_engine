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
                               VkImageLayout initial_layout,
                               VkImageLayout final_layout,
                               VkAttachmentLoadOp load_operation,
                               VkAttachmentStoreOp store_operation)
        : _texture_views(std::move(texture_views))
        , _extent{ .width = width, .height = height }
        , _image_format(image_format)
        , _initial_layout(initial_layout)
        , _final_layout(final_layout)
        , _load_operation(load_operation)
        , _store_operation(store_operation)
    {}
    const Image& RenderTarget::getImage(uint32_t index) const
    {
        return _texture_views[index]->getTexture().getImage();
    }
    RenderTarget::~RenderTarget() = default;

}