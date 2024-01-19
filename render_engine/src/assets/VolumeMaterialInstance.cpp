#include <render_engine/assets/VolumeMaterialInstance.h>

namespace RenderEngine
{

    VolumeMaterialInstance::CloneResult VolumeMaterialInstance::cloneForFrontFacePass(std::unique_ptr<Shader> fragment_shader, int32_t id) const
    {
        CloneResult result;
        const auto& material = static_cast<const VolumeMaterial&>(getMaterial());
        result.material = material.createForFrontFace(std::move(fragment_shader),
                                                      id);
        result.instance = std::unique_ptr<VolumeMaterialInstance>(new VolumeMaterialInstance(*result.material,
                                                                                             _intensity_texture,
                                                                                             _segmentation_threshold,
                                                                                             TextureBindingMap{},
                                                                                             getCallbackContainer(),
                                                                                             id));
        return result;
    }
    VolumeMaterialInstance::CloneResult VolumeMaterialInstance::cloneForBackFacePass(std::unique_ptr<Shader> fragment_shader, int32_t id) const
    {
        CloneResult result;
        const auto& material = static_cast<const VolumeMaterial&>(getMaterial());
        result.material = material.createForBackFace(std::move(fragment_shader),
                                                     id);
        result.instance = std::unique_ptr<VolumeMaterialInstance>(new VolumeMaterialInstance(*result.material,
                                                                                             _intensity_texture,
                                                                                             _segmentation_threshold,
                                                                                             TextureBindingMap{},
                                                                                             getCallbackContainer(),
                                                                                             id));
        return result;
    }

}