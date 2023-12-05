#include "render_engine/assets/Image.h"

#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RenderEngine
{
    Image::Image(const std::filesystem::path& path)
    {
        int32_t width{ -1 };
        int32_t height{ -1 };
        int32_t channels{ -1 };
        std::string path_str = path.string();
        stbi_uc* pixels = stbi_load(path_str.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (channels >= 3)
        {
            _width = width;
            _height = height;
            _format = VK_FORMAT_R8G8B8A8_SRGB;
            _data = std::vector<uint8_t>(pixels, pixels + (width * height * 4));
            stbi_image_free(pixels);
        }
        else
        {
            throw std::runtime_error("Unsupported image format");
        }
    }


    std::vector<uint8_t> Image::createEmptyData() const
    {
        int32_t width{ -1 };
        int32_t height{ -1 };
        int32_t channels{ -1 };
        if (_format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            std::vector<uint8_t> result(width * height * 4, static_cast<uint8_t>(0u));
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
