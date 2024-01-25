#include <render_engine/assets/TextureBindingMap.h>

#include <render_engine/containers/VariantOverloaded.h>

namespace RenderEngine
{
    std::unordered_map<int32_t, std::vector<ITextureView*>> TextureBindingMap::collectTextureViews(int32_t back_buffer_size) const
    {
        std::unordered_map<int32_t, std::vector<ITextureView*>> result;
        for (auto& [binding, texture_view] : _general_texture_bindings)
        {
            result[binding] = std::vector(back_buffer_size, texture_view.get());
        }
        for (auto& [binding, texture_views] : _back_buffered_texture_bindings)
        {
            assert(result.contains(binding) == false
                   && ("Somehow the binding is already occupied. Something went wrong with the setup."
                       " This should only happen with a merge operation but that should protect the overlapping bindings."));
            std::ranges::transform(texture_views, std::back_inserter(result[binding]),
                                   [](auto& ptr) { return ptr.get(); });
        }
        return result;
    }

    TextureBindingMap TextureBindingMap::clone() const
    {
        TextureBindingMap clone;
        for (auto& [binding, texture_view] : _general_texture_bindings)
        {
            clone._general_texture_bindings[binding] = texture_view->clone();
        }
        for (auto& [binding, texture_views] : _back_buffered_texture_bindings)
        {
            for (const auto& texture_view : texture_views)
            {
                clone._back_buffered_texture_bindings[binding].push_back(texture_view->clone());
            }
        }
        return clone;
    }


}