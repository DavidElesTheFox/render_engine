#pragma once

#include <render_engine/assets/Shader.h>
#include <render_engine/resources/Texture.h>

#include <unordered_map>
#include <variant>

namespace RenderEngine
{
    class TextureBindingMapView;

    class TextureBindingMap
    {
    public:
        TextureBindingMap() = default;
        explicit TextureBindingMap(std::unordered_map<int32_t, std::unique_ptr<ITextureView>> general_bindings)
            : _general_texture_bindings(std::move(general_bindings))
        {}
        explicit TextureBindingMap(std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> back_buffered_bindings)
            : _back_buffered_texture_bindings(std::move(back_buffered_bindings))
        {}

        std::unordered_map<int32_t, std::vector<ITextureView*>> createTextureViews(int32_t back_buffer_size) const;

        TextureBindingMap clone() const;
    private:
        std::unordered_map<int32_t, std::unique_ptr<ITextureView>> _general_texture_bindings;
        std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> _back_buffered_texture_bindings;
    };


}