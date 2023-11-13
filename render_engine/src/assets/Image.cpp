#include "render_engine/assets/Image.h"

#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RenderEngine
{
    std::vector<uint8_t> Image::readData() const
    {
        int32_t width{ -1 };
        int32_t height{ -1 };
        int32_t channels{ -1 };
        std::string path_str = _path.string();
        if (_format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            stbi_uc* pixels = stbi_load(path_str.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            std::vector<uint8_t> result(pixels, pixels + (width * height * 4));
            stbi_image_free(pixels);    
            return result;
        }
        else
        {
            throw std::runtime_error("Unsupported image format");
        }
    }
    BufferInfo Image::createBufferInfo() const
    {
        return BufferInfo{
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .size = getSize(),
                .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                .mapped = true
        };
    }
    VkDeviceSize Image::getSize() const
    {
        VkDeviceSize result = getWidth() * getHeight();
        switch (_format)
        {
        case VK_FORMAT_R8G8B8A8_SRGB:
            return result *= 4;
        default:
            throw std::runtime_error("Unhandled image format");
        }
        return result;
    }
}
