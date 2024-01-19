#pragma once

#include <render_engine/assets/Image.h>
#include <render_engine/assets/VolumeMaterial.h>
#include <render_engine/resources/Texture.h>
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
                               Texture& intensity_texture,
                               uint32_t segmentation_threshold,
                               TextureBindingMap texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id)
            : MaterialInstance(material, std::move(texture_bindings), std::move(callbacks), id)
            , _intensity_texture(intensity_texture)
            , _segmentation_threshold(segmentation_threshold)
        {}
        ~VolumeMaterialInstance() = default;

        CloneResult cloneForFrontFacePass(std::unique_ptr<Shader> fragment_shader,
                                          int32_t id) const;
        CloneResult cloneForBackFacePass(std::unique_ptr<Shader> fragment_shader,
                                         int32_t id) const;

        const VolumeMaterial& getVolumeMaterial() const { return static_cast<const VolumeMaterial&>(getMaterial()); }

        const Image& getIntensityImage() const { return _intensity_texture.getImage(); }
        const Texture& getIntensityTexture() const
        {
            return _intensity_texture;
        }

        uint32_t getSegmentationThreshold() const { return _segmentation_threshold; }
    private:
        VolumeMaterialInstance(Material& material,
                               Texture& intensity_texture,
                               uint32_t segmentation_threshold,
                               TextureBindingMap texture_bindings,
                               CallbackContainer callbacks,
                               uint32_t id)
            : MaterialInstance(material, std::move(texture_bindings), std::move(callbacks), id)
            , _intensity_texture(intensity_texture)
            , _segmentation_threshold(segmentation_threshold)
        {}
        Texture& _intensity_texture;
        uint32_t _segmentation_threshold{};
    };
}