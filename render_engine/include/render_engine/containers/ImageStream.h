#pragma once

#include <deque>
#include <iterator>
#include <shared_mutex>
#include <vector>

#include <volk.h>

namespace RenderEngine
{
    class ImageStream
    {
    public:
        struct ImageDescription
        {
            uint32_t width{ 0 };
            uint32_t height{ 0 };
            VkFormat format{ VK_FORMAT_UNDEFINED };
        };

        explicit ImageStream(ImageDescription image_description)
            : _image_description(std::move(image_description))
        {

        }
        ImageStream& operator<<(std::vector<uint8_t> data)
        {
            std::unique_lock lock(_image_data_container_mutex);
            _image_data_container.emplace_back(std::move(data));
            return *this;
        }

        ImageStream& operator>>(std::vector<uint8_t>& output)
        {
            if (_image_data_container.empty())
            {
                return *this;
            }

            std::unique_lock lock(_image_data_container_mutex);

            output = std::move(_image_data_container.front());
            _image_data_container.pop_front();
            return *this;

        }

        bool isEmpty() const
        {
            std::shared_lock lock(_image_data_container_mutex);
            return _image_data_container.empty();
        }

        const ImageDescription getImageDescription() const { return _image_description; }
    private:
        ImageDescription _image_description;
        std::deque<std::vector<uint8_t>> _image_data_container;
        mutable std::shared_mutex _image_data_container_mutex;
    };
}