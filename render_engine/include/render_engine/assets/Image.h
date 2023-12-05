#pragma once

#include <volk.h>

#include <filesystem>

#include <render_engine/resources/Buffer.h>

namespace RenderEngine
{
    class Image
    {
    public:
        Image(const std::filesystem::path& path);
        Image(uint32_t width,
              uint32_t height,
              VkFormat format)
            : _width(width)
            , _height(height)
            , _format(format)
        {
            _data = createEmptyData();
        }
        uint32_t getWidth() const { return _width; }
        uint32_t getHeight() const { return _height; }
        VkFormat getFormat() const { return _format; }
        BufferInfo createBufferInfo() const;
        const std::vector<uint8_t>& getData() const { return _data; }
        void setData(std::vector<uint8_t> value) { _data = std::move(value); }
        VkDeviceSize getSize() const;
    private:

        std::vector<uint8_t> createEmptyData() const;
        uint32_t _width{ 0 };
        uint32_t _height{ 0 };
        VkFormat _format{ VK_FORMAT_UNDEFINED };
        std::vector<uint8_t> _data;
    };
}