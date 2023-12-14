#pragma once

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
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
                                                     uint32_t id) const
        {
            std::unique_ptr<Shader> vertex_shader = std::make_unique<Shader>(getVertexShader());
            std::unique_ptr<Material> result(new Material(std::move(vertex_shader),
                                                          std::move(fragment_shader),
                                                          { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                                                          id));
            result->setRasterizationInfo(result->getRasterizationInfo().clone()
                                         .setCullMode(VK_CULL_MODE_BACK_BIT));
            return result;
        }
        std::unique_ptr<Material> createForBackFace(std::unique_ptr<Shader> fragment_shader,
                                                    uint32_t id) const
        {
            std::unique_ptr<Shader> vertex_shader = std::make_unique<Shader>(getVertexShader());
            std::unique_ptr<Material> result(new Material(std::move(vertex_shader),
                                                          std::move(fragment_shader),
                                                          { [](const Geometry& geometry, const Material& material) { return createVertexBuffer(geometry, material); } },
                                                          id));
            result->setRasterizationInfo(result->getRasterizationInfo().clone()
                                         .setCullMode(VK_CULL_MODE_FRONT_BIT));
            return result;
        }
        ~VolumeMaterial() override = default;
    private:

        static std::vector<uint8_t> createVertexBuffer(const Geometry& geometry, const Material&);
    };

    class VolumeMaterialInstance : public MaterialInstance
    {
    public:
        struct CloneResult
        {
            std::unique_ptr<MaterialInstance> instance;
            std::unique_ptr<Material> material;
        };
        VolumeMaterialInstance(VolumeMaterial& material,
                               TextureBindingData texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id)
            : MaterialInstance(material, std::move(texture_bindings), std::move(callbacks), id)
        {}
        ~VolumeMaterialInstance() = default;

        CloneResult cloneForFrontFacePass(std::unique_ptr<Shader> fragment_shader,
                                          int32_t id) const
        {
            CloneResult result;
            const auto& material = static_cast<const VolumeMaterial&>(getMaterial());
            result.material = material.createForFrontFace(std::move(fragment_shader),
                                                          id);
            result.instance = std::unique_ptr<VolumeMaterialInstance>(new VolumeMaterialInstance(*result.material,
                                                                                                 TextureBindingData{},
                                                                                                 getCallbackContainer(),
                                                                                                 id));
            return result;
        }
        CloneResult cloneForBackFacePass(std::unique_ptr<Shader> fragment_shader,
                                         int32_t id) const
        {
            CloneResult result;
            const auto& material = static_cast<const VolumeMaterial&>(getMaterial());
            result.material = material.createForBackFace(std::move(fragment_shader),
                                                         id);
            result.instance = std::unique_ptr<VolumeMaterialInstance>(new VolumeMaterialInstance(*result.material,
                                                                                                 TextureBindingData{},
                                                                                                 getCallbackContainer(),
                                                                                                 id));
            return result;
        }
    private:
        VolumeMaterialInstance(Material& material,
                               TextureBindingData texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id)
            : MaterialInstance(material, std::move(texture_bindings), std::move(callbacks), id)
        {}
    };
}