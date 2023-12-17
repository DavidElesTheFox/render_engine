#include "render_engine/assets/Image.h"

#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RenderEngine
{
    namespace
    {
        struct ImageLoadResult
        {
            int32_t width{ -1 };
            int32_t height{ -1 };
            int32_t channels{ -1 };
            stbi_uc* pixels{ nullptr };
            ImageLoadResult() = default;
            ImageLoadResult(ImageLoadResult&& o) noexcept
                : width(o.width)
                , height(o.height)
                , channels(o.channels)
                , pixels(o.pixels)
            {
                o.pixels = nullptr;
            }
            ImageLoadResult& operator=(ImageLoadResult&&) = delete;
            ImageLoadResult(const ImageLoadResult&) = delete;
            ImageLoadResult& operator=(const ImageLoadResult&) = delete;
            ~ImageLoadResult()
            {
                if (pixels != nullptr)
                {
                    stbi_image_free(pixels);
                    pixels = nullptr;
                }
            }
        };

        ImageLoadResult loadImage(const std::filesystem::path& path)
        {
            ImageLoadResult result;
            std::string path_str = path.string();
            result.pixels = stbi_load(path_str.c_str(), &result.width, &result.height, &result.channels, STBI_rgb_alpha);
            if (result.pixels == nullptr)
            {
                throw std::runtime_error("File not found: " + path.string());
            }
            if (result.channels < 3)
            {
                throw std::runtime_error("Unsupported image format");
            }
            return result;
        }
    }
    Image::Image(const std::filesystem::path& path)
    {
        ImageLoadResult image_data = loadImage(path);
        _width = image_data.width;
        _height = image_data.height;
        _format = VK_FORMAT_R8G8B8A8_SRGB;
        _data.resize(getSize());
        std::copy(image_data.pixels, image_data.pixels + _data.size(), _data.data());
    }

    Image::Image(const std::vector<std::filesystem::path>& path_container)

    {
        std::vector<ImageLoadResult> loaded_images;
        for (const auto& path : path_container)
        {
            loaded_images.push_back(loadImage(path));
        }
        _width = loaded_images[0].width;
        _height = loaded_images[0].height;
        _format = VK_FORMAT_R8G8B8A8_SRGB;
        _depth = path_container.size();
        _data.resize(getSize());

        const auto index_at_3d = [&](uint32_t u, uint32_t v, uint32_t s) { return 4 * (u + v * _width + s * (_width * _height)); };
        const auto index_at_2d = [&](uint32_t u, uint32_t v) { return (u + v * _width) * 4; };
        for (uint32_t s = 0; s < _depth; ++s)
        {
            for (uint32_t v = 0; v < _height; ++v)
            {
                for (uint32_t u = 0; u < _width; ++u)
                {
                    _data[index_at_3d(u, v, s)] = loaded_images[s].pixels[index_at_2d(u, v)];
                    _data[index_at_3d(u, v, s) + 1] = loaded_images[s].pixels[index_at_2d(u, v) + 1];
                    _data[index_at_3d(u, v, s) + 2] = loaded_images[s].pixels[index_at_2d(u, v) + 2];
                    _data[index_at_3d(u, v, s) + 3] = loaded_images[s].pixels[index_at_2d(u, v) + 3];
                }
            }
        }
    }

    std::vector<uint8_t> Image::createEmptyData() const
    {
        std::vector<uint8_t> result(getSize(), static_cast<uint8_t>(0u));
        return result;
    }
    BufferInfo Image::createBufferInfo() const
    {
        return BufferInfo{
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .size = getSize(),
                .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                .mapped = true
        };
    }
    VkDeviceSize Image::getSize() const
    {
        VkDeviceSize result = getWidth() * getHeight() * _depth;
        switch (_format)
        {
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return result *= 4;
            default:
                throw std::runtime_error("Unhandled image format");
        }
        return result;
    }

    void Image::processData(const ImageProcessor& image_processor)
    {
        if (is3D() != image_processor.is3DProcessor())
        {
            throw std::runtime_error("Image processor type error");
        }
        if (is3D())
        {
            DataAccessor3D accessor(*this);
            image_processor.get3DCallback()(_width, _height, _depth, accessor);
        }
        else
        {
            DataAccessor2D accessor(*this);
            image_processor.get2DCallback()(_width, _height, accessor);
        }
    }
}
