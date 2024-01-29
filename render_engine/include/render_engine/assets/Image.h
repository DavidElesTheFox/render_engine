#pragma once

#include <volk.h>

#include <filesystem>
#include <variant>

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
                assert(_image.getPixelComponentCount() == 4 && "Currently only 4 component is supported");
                const uint32_t component_count = _image.getPixelComponentCount();
                const uint32_t row_index = (u + v * _image._width) * component_count;
                const auto& image_data = std::get<std::vector<uint8_t>>(_image._data);
                return { image_data[row_index], image_data[row_index + 1], image_data[row_index + 2], image_data[row_index + 3] };
            }
            void setPixel(uint32_t u, uint32_t v, uint32_t s, const glm::vec4& data)
            {
                assert(_image.getPixelComponentCount() == 4 && "Currently only 4 component is supported");

                const uint32_t component_count = _image.getPixelComponentCount();
                const uint32_t row_index = (u + v * _image._width) * component_count;
                auto& image_data = std::get<std::vector<uint8_t>>(_image._data);

                image_data[row_index] = data.r;
                image_data[row_index + 1] = data.g;
                image_data[row_index + 2] = data.b;
                image_data[row_index + 3] = data.a;
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
                assert(_image.getPixelComponentCount() == 4 && "Currently only 4 component is supported");

                const uint32_t component_count = _image.getPixelComponentCount();
                const auto row_index = (u + v * _image._width + s * (_image._width * _image._height)) * component_count;
                const auto& image_data = std::get<std::vector<uint8_t>>(_image._data);
                return { image_data[row_index], image_data[row_index + 1], image_data[row_index + 2], image_data[row_index + 3] };
            }
            void setPixel(uint32_t u, uint32_t v, uint32_t s, const glm::vec4& data)
            {
                assert(_image.getPixelComponentCount() == 4 && "Currently only 4 component is supported");

                const uint32_t component_count = _image.getPixelComponentCount();

                const auto row_index = (u + v * _image._width + s * (_image._width * _image._height)) * component_count;
                auto& image_data = std::get<std::vector<uint8_t>>(_image._data);

                image_data[row_index] = data.r;
                image_data[row_index + 1] = data.g;
                image_data[row_index + 2] = data.b;
                image_data[row_index + 3] = data.a;
            }
        private:
            explicit DataAccessor3D(Image& image)
                : _image(image)
            {}
            Image& _image;
        };
#pragma endregion

        using RawData = std::variant<std::vector<float>, std::vector<uint8_t>>;
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

        Image(const Image&) = default;
        Image& operator=(const Image&) = default;
        Image(Image&&) = default;
        Image& operator=(Image&&) = default;
        uint32_t getWidth() const { return _width; }
        uint32_t getHeight() const { return _height; }
        uint32_t getDepth() const { return _depth; }

        VkFormat getFormat() const { return _format; }

        bool is3D() const { return _depth != 1; }

        BufferInfo createBufferInfo() const;
        const RawData& getData() const { return _data; }
        void setData(std::vector<uint8_t> value) { _data = std::move(value); }
        VkDeviceSize getSize() const;

        void processData(const ImageProcessor& image_processor);
        void saveRawDataToFile(const std::filesystem::path& file_path) const;
    private:
        uint32_t getPixelComponentCount() const;
        RawData createEmptyData() const;
        uint32_t _width{ 0 };
        uint32_t _height{ 0 };
        uint32_t _depth{ 1 };
        VkFormat _format{ VK_FORMAT_UNDEFINED };
        RawData _data;
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