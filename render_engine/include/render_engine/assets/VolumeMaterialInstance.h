#pragma once

#include <render_engine/assets/VolumeMaterial.h>
namespace RenderEngine
{
    class VolumeMaterialInstance : public MaterialInstance
    {
    public:
        struct CloneResult
        {
            std::unique_ptr<MaterialInstance> instance;
            std::unique_ptr<Material> material;
        };
        VolumeMaterialInstance(VolumeMaterial& material,
                               TextureBindingMap texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id)
            : MaterialInstance(material, std::move(texture_bindings), std::move(callbacks), id)
        {}
        ~VolumeMaterialInstance() = default;

        CloneResult cloneForFrontFacePass(std::unique_ptr<Shader> fragment_shader,
                                          int32_t id) const;
        CloneResult cloneForBackFacePass(std::unique_ptr<Shader> fragment_shader,
                                         int32_t id) const;
    private:
        VolumeMaterialInstance(Material& material,
                               TextureBindingMap texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id)
            : MaterialInstance(material, std::move(texture_bindings), std::move(callbacks), id)
        {}
    };
}