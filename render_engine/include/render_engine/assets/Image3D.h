#pragma once

#include <render_engine/assets/Image.h>

namespace RenderEngine
{
    class Image3D
    {
    public:
        Image3D(const std::vector<std::filesystem::path>& image_paths, uint32_t slice_distance);
        Image3D(std::vector<Image> images, uint32_t slice_distance)
            : _images(std::move(images))
            , _slice_distance(slice_distance)
        {

        }
        ~Image3D() = default;
        const std::vector<Image>& getImages() const { return _images; }
        uint32_t getSliceDistance() const { return _slice_distance; }
    private:
        std::vector<Image> _images;
        uint32_t _slice_distance{ 0 };
    };
}