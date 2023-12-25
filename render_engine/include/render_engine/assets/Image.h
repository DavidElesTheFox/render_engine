#pragma once

#include <volk.h>

#include <filesystem>

#include <render_engine/resources/Buffer.h>

#include <glm/vec4.hpp>
namespace RenderEngine
{
    class ImageProcessor;
    class Image
    {
    public:

#pragma region Accessors
        friend class DataAccessor2D;
        friend class DataAccessor3D;


        class DataAccessor2D
        {
        public:
            friend class Image;
            glm::vec4 getPixel(uint32_t u, uint32_t v)
            {
                return {
                    _image._data[(u + v * _image._width) * 4],
                    _image._data[(u + v * _image._width) * 4 + 1],
                    _image._data[(u + v * _image._width) * 4 + 2],
                    _image._data[(u + v * _image._width) * 4 + 3]
                };
            }
            void setPixel(uint32_t u, uint32_t v, uint32_t s, const glm::vec4& data)
            {
                _image._data[(u + v * _image._width) * 4] = data.r;
                _image._data[(u + v * _image._width) * 4 + 1] = data.g;
                _image._data[(u + v * _image._width) * 4 + 2] = data.b;
                _image._data[(u + v * _image._width) * 4 + 3] = data.a;
            }
        private:
            explicit DataAccessor2D(Image& image)
                : _image(image)
            {}
            Image& _image;
        };
        class DataAccessor3D
        {
        public:
            friend class Image;
            glm::vec4 getPixel(uint32_t u, uint32_t v, uint32_t s)
            {
                const auto row_index = (u + v * _image._width + s * (_image._width * _image._height)) * 4;
                return {
                    _image._data[row_index],
                    _image._data[row_index + 1],
                    _image._data[row_index + 2],
                    _image._data[row_index + 3]
                };
            }
            void setPixel(uint32_t u, uint32_t v, uint32_t s, const glm::vec4& data)
            {
                const auto row_index = (u + v * _image._width + s * (_image._width * _image._height)) * 4;
                _image._data[row_index] = data.r;
                _image._data[row_index + 1] = data.g;
                _image._data[row_index + 2] = data.b;
                _image._data[row_index + 3] = data.a;
            }
        private:
            explicit DataAccessor3D(Image& image)
                : _image(image)
            {}
            Image& _image;
        };
#pragma endregion

        explicit Image(const std::filesystem::path& path);
        explicit Image(const std::vector<std::filesystem::path>& path_container);
        Image(uint32_t width,
              uint32_t height,
              VkFormat format)
            : _width(width)
            , _height(height)
            , _format(format)
        {
            _data = createEmptyData();
        }

        Image(uint32_t width,
              uint32_t height,
              uint32_t depth,
              VkFormat format)
            : _width(width)
            , _height(height)
            , _depth(depth)
            , _format(format)
        {
            _data = createEmptyData();
        }
        uint32_t getWidth() const { return _width; }
        uint32_t getHeight() const { return _height; }
        uint32_t getDepth() const { return _depth; }

        VkFormat getFormat() const { return _format; }

        bool is3D() const { return _depth != 1; }

        BufferInfo createBufferInfo() const;
        const std::vector<uint8_t>& getData() const { return _data; }
        void setData(std::vector<uint8_t> value) { _data = std::move(value); }
        VkDeviceSize getSize() const;

        void processData(const ImageProcessor& image_processor);
    private:

        std::vector<uint8_t> createEmptyData() const;
        uint32_t _width{ 0 };
        uint32_t _height{ 0 };
        uint32_t _depth{ 1 };
        VkFormat _format{ VK_FORMAT_UNDEFINED };
        std::vector<uint8_t> _data;
    };


    class ImageProcessor
    {
    public:
        explicit ImageProcessor(std::function<void(uint32_t, uint32_t, Image::DataAccessor2D&)> callback)
            : _2d_callback(std::move(callback))
        {}
        explicit ImageProcessor(std::function<void(uint32_t, uint32_t, uint32_t, Image::DataAccessor3D&)> callback)
            : _3d_callback(std::move(callback))
        {}
        const std::function<void(uint32_t, uint32_t, Image::DataAccessor2D&)>& get2DCallback() const
        {
            return _2d_callback;
        }
        const std::function<void(uint32_t, uint32_t, uint32_t, Image::DataAccessor3D&)>& get3DCallback() const
        {
            return _3d_callback;
        }

        bool is3DProcessor() const { return _3d_callback != nullptr; }
    private:
        std::function<void(uint32_t, uint32_t, Image::DataAccessor2D&)> _2d_callback{ nullptr };
        std::function<void(uint32_t, uint32_t, uint32_t, Image::DataAccessor3D&)> _3d_callback{ nullptr };
    };
}