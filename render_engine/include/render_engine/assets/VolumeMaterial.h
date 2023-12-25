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
                       uint32_t id)
            : Material(std::move(vertex_shader),
                       std::move(fragment_shader),
                       { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                       id)
        {}
        std::unique_ptr<Material> createForFrontFace(std::unique_ptr<Shader> fragment_shader,
                                                     uint32_t id) const;
        std::unique_ptr<Material> createForBackFace(std::unique_ptr<Shader> fragment_shader,
                                                    uint32_t id) const;
        ~VolumeMaterial() override = default;
    private:

        static std::vector<uint8_t> createVertexBuffer(const Geometry& geometry, const Material&);
    };


}