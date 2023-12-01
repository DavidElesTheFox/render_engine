#pragma once

#include <volk.h>

#include <filesystem>

#include <render_engine/resources/Buffer.h>

namespace RenderEngine
{
    class Image
    {
    public:
        Image(std::filesystem::path path,
              uint32_t width,
              uint32_t height,
              VkFormat format)
            : _path(path)
            , _width(width)
            , _height(height)
            , _format(format)
        {}

        const std::filesystem::path& getPath() const { return _path; }
        uint32_t getWidth() const { return _width; }
        uint32_t getHeight() const { return _height; }
        VkFormat getFormat() const { return _format; }
        std::vector<uint8_t> readData() const;
        BufferInfo createBufferInfo() const;

        VkDeviceSize getSize() const;
    private:
        std::filesystem::path _path;
        uint32_t _width{ 0 };
        uint32_t _height{ 0 };
        VkFormat _format{ VK_FORMAT_UNDEFINED };
    };
}