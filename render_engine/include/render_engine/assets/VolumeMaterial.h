#pragma once

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/MaterialInstance.h>
#include <render_engine/assets/VolumeShader.h>

namespace RenderEngine
{
    class VolumeMaterial : public Material
    {
    public:
        VolumeMaterial(std::unique_ptr<VolumeShader> vertex_shader,
                       std::unique_ptr<VolumeShader> fragment_shader,
                       bool require_distance_field,
                       uint32_t id,
                       std::string name)
            : Material(std::move(vertex_shader),
                       std::move(fragment_shader),
                       { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                       id,
                       std::move(name))
            , _require_distance_field(require_distance_field)
        {}
        std::unique_ptr<Material> createForFrontFace(std::unique_ptr<Shader> fragment_shader,
                                                     uint32_t id) const;
        std::unique_ptr<Material> createForBackFace(std::unique_ptr<Shader> fragment_shader,
                                                    uint32_t id) const;
        ~VolumeMaterial() override = default;
        bool isRequireDistanceField() const { return _require_distance_field; }
        void setRequireDistanceField(bool value) { _require_distance_field = value; }
    private:

        static std::vector<uint8_t> createVertexBuffer(const Geometry& geometry, const Material&);

        bool _require_distance_field{ false };
    };


}