#pragma once

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/VolumeShader.h>

namespace RenderEngine
{
    class VolumeMaterial : public Material
    {
    public:
        std::unique_ptr<VolumeMaterial> createForFrontFace(VolumeShader vertex_shader,
                                                           VolumeShader fragment_shader,
                                                           uint32_t id)
        {
            std::unique_ptr<VolumeMaterial> result(new VolumeMaterial(std::move(vertex_shader),
                                                                      std::move(fragment_shader),
                                                                      id));
            result->setRasterizationInfo(result->getRasterizationInfo().clone()
                                         .setCullMode(VK_CULL_MODE_BACK_BIT));
            return result;
        }
        std::unique_ptr<VolumeMaterial> createForBackFace(VolumeShader vertex_shader,
                                                          VolumeShader fragment_shader,
                                                          uint32_t id)
        {
            std::unique_ptr<VolumeMaterial> result(new VolumeMaterial(std::move(vertex_shader),
                                                                      std::move(fragment_shader),
                                                                      id));
            result->setRasterizationInfo(result->getRasterizationInfo().clone()
                                         .setCullMode(VK_CULL_MODE_FRONT_BIT));
            return result;
        }
        ~VolumeMaterial() override = default;
    private:
        VolumeMaterial(VolumeShader vertex_shader,
                       VolumeShader fragment_shader,
                       uint32_t id)
            : Material(std::move(vertex_shader),
                       std::move(fragment_shader),
                       { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                       id)
        {}
        static std::vector<uint8_t> createVertexBuffer(const Geometry& geometry, const Material&);
    };

    class VolumeMaterialInstance : public MaterialInstance
    {
    public:
        VolumeMaterialInstance(VolumeMaterial& material,
                               TextureBindingData texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id);
        ~VolumeMaterialInstance() = default;
    };
}