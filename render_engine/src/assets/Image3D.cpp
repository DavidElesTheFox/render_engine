#include <render_engine/assets/Image3D.h>

#include <ranges>

namespace RenderEngine
{
    Image3D::Image3D(const std::vector<std::filesystem::path>& image_paths, uint32_t slice_distance)
        : _slice_distance(slice_distance)
    {
        std::ranges::transform(image_paths,
                               std::back_inserter(_images),
                               [](const auto& path) { return Image(path); });
    }
}