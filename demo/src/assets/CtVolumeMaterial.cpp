#include <assets/CtVolumeMaterial.h>

#include <render_engine/assets/Mesh.h>
#include <render_engine/resources/PushConstantsUpdater.h>


#include <demo/resource_config.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>
#include <scene/Scene.h>
#include <scene/SceneNodeLookup.h>
#include <scene/VolumeObject.h>

namespace Assets
{
    CtVolumeMaterial::CtVolumeMaterial(uint32_t id)
    {
        using namespace RenderEngine;

        VolumeShader::MetaDataExtension vertex_meta_data = VolumeShader::MetaDataExtension::createForVertexShader();
        {
            VolumeShader::MetaData::PushConstants push_constants{ .size = sizeof(VertexPushConstants),.offset = 0 };
            vertex_meta_data.setPushConstants(std::move(push_constants));
        }

        VolumeShader::MetaDataExtension frament_meta_data = VolumeShader::MetaDataExtension::createForFragmentShader();
        frament_meta_data.addSampler(2, Shader::MetaData::Sampler{ .binding = 2 });


        std::filesystem::path base_path = SHADER_BASE;
        auto vertex_shader = std::make_unique<VolumeShader>(base_path / "ct_volume.vert.spv", vertex_meta_data);
        auto fretment_shader = std::make_unique<VolumeShader>(base_path / "ct_volume.frag.spv", frament_meta_data);

        _material = std::make_unique<VolumeMaterial>(std::move(vertex_shader),
                                                     std::move(fretment_shader),
                                                     id);
        _material->setColorBlending(_material->getColorBlending().clone()
                                    .setEnabled(true));
        _material->setAlphaBlending(_material->getAlpheBlending().clone()
                                    .setEnabled(true));
    }
    std::unique_ptr<CtVolumeMaterial::Instance> CtVolumeMaterial::createInstance(std::unique_ptr<RenderEngine::ITextureView> texture_view, Scene::Scene* scene, uint32_t id)
    {
        using namespace RenderEngine;
        std::unique_ptr<Instance> result = std::make_unique<Instance>();
        std::unordered_map<int32_t, std::unique_ptr<ITextureView>> texture_map;
        texture_map[2] = std::move(texture_view);

        auto global_push_constat_update = [material_constants = &result->_material_constants, scene](PushConstantsUpdater& updater)
            {
                material_constants->vertex_values.projection = scene->getActiveCamera()->getProjection();
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.projection), sizeof(material_constants->vertex_values.projection));
                    updater.update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, projection), data_view);
                }
            };
        auto push_constant_update = [material_constants = &result->_material_constants, scene](const MeshInstance* mesh, PushConstantsUpdater& updater)
            {
                auto model = scene->getNodeLookup().findVolumObject(mesh->getId())->getTransformation().calculateTransformation();
                auto view = scene->getActiveCamera()->getView();
                material_constants->vertex_values.model_view = view * model;

                const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.model_view), sizeof(material_constants->vertex_values.model_view));
                updater.update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, model_view), data_view);
            };
        result->_material_instance = std::make_unique<VolumeMaterialInstance>(*_material,
                                                                              VolumeMaterialInstance::TextureBindingData(std::move(texture_map)),
                                                                              VolumeMaterialInstance::CallbackContainer{
                                                                                  .global_ubo_update = [](std::vector<UniformBinding>& , uint32_t) {},
                                                                                  .global_push_constants_update = global_push_constat_update ,
                                                                                  .push_constants_updater = push_constant_update },
                                                                                  id);
        return result;
    }
    void CtVolumeMaterial::processImage(RenderEngine::Image* image) const
    {
        auto segmentation_callback = [&](uint32_t width, uint32_t height, uint32_t depth, RenderEngine::Image::DataAccessor3D& accessor)
            {
                for (uint32_t s = 0; s < depth; ++s)
                {
                    for (uint32_t v = 0; v < height; ++v)
                    {
                        for (uint32_t u = 0; u < width; ++u)
                        {
                            glm::vec4 intensity = accessor.getPixel(u, v, s);
                            glm::vec4 color(0.0f, 0.0f, 0.0f, 0.0f);

                            // skip no gray scale values
                            if (intensity.r != intensity.g
                                || intensity.r != intensity.b)
                            {
                                accessor.setPixel(u, v, s, color);
                                continue;
                            }

                            for (const auto& segmentation : _segmentations)
                            {

                                if (intensity.r > segmentation.threshold)
                                {
                                    color = segmentation.color * 255.0f;
                                }
                            }
                            accessor.setPixel(u, v, s, color);
                        }
                    }
                }
            };
        image->processData(RenderEngine::ImageProcessor{ std::move(segmentation_callback) });
    }
    void CtVolumeMaterial::addSegmentation(SegmentationData segmentation)
    {
        _segmentations.push_back(std::move(segmentation));
        std::ranges::stable_sort(_segmentations, [](const auto& a, const auto& b) { return a.threshold < b.threshold; });
    }
}