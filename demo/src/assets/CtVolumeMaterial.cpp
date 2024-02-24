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
    CtVolumeMaterial::CtVolumeMaterial(bool use_ao, uint32_t id)
        : _use_ao(use_ao)
    {
        using namespace RenderEngine;

        VolumeShader::MetaDataExtension vertex_meta_data = VolumeShader::MetaDataExtension::createForVertexShader();
        {
            VolumeShader::MetaData::PushConstants push_constants{ .size = sizeof(VertexPushConstants),.offset = 0, .update_frequency = Shader::MetaData::UpdateFrequency::PerDrawCall };
            vertex_meta_data.setPushConstants(std::move(push_constants));
        }

        VolumeShader::MetaDataExtension frament_meta_data = VolumeShader::MetaDataExtension::createForFragmentShader(use_ao);
        {
            VolumeShader::MetaData::PushConstants push_constants{ .size = sizeof(FragmentPushConstants),.offset = sizeof(VertexPushConstants), .update_frequency = Shader::MetaData::UpdateFrequency::PerFrame };
            frament_meta_data.setPushConstants(std::move(push_constants));
        }
        frament_meta_data.addSampler(2, Shader::MetaData::Sampler{ .binding = 2, .update_frequency = Shader::MetaData::UpdateFrequency::PerFrame });
        if (use_ao)
        {
            frament_meta_data.addSampler(3, Shader::MetaData::Sampler{ .binding = 3, .update_frequency = Shader::MetaData::UpdateFrequency::PerFrame });


            constexpr bool require_distance_filed = true;
            std::filesystem::path base_path = SHADER_BASE;
            auto vertex_shader = std::make_unique<VolumeShader>(base_path / "ct_volume_ao.vert.spv", vertex_meta_data);
            auto fretment_shader = std::make_unique<VolumeShader>(base_path / "ct_volume_ao.frag.spv", frament_meta_data);

            _material = std::make_unique<VolumeMaterial>(std::move(vertex_shader),
                                                         std::move(fretment_shader),
                                                         require_distance_filed,
                                                         id,
                                                         "CtVolumeMaterialAO");
            _material->setColorBlending(_material->getColorBlending().clone()
                                        .setEnabled(true));
            _material->setAlphaBlending(_material->getAlpheBlending().clone()
                                        .setEnabled(true));
        }
        else
        {
            constexpr bool require_distance_filed = false;
            std::filesystem::path base_path = SHADER_BASE;
            auto vertex_shader = std::make_unique<VolumeShader>(base_path / "ct_volume.vert.spv", vertex_meta_data);
            auto fretment_shader = std::make_unique<VolumeShader>(base_path / "ct_volume.frag.spv", frament_meta_data);

            _material = std::make_unique<VolumeMaterial>(std::move(vertex_shader),
                                                         std::move(fretment_shader),
                                                         require_distance_filed,
                                                         id,
                                                         "CtVolumeMaterial");
            _material->setColorBlending(_material->getColorBlending().clone()
                                        .setEnabled(true));
            _material->setAlphaBlending(_material->getAlpheBlending().clone()
                                        .setEnabled(true));
        }
    }
    std::unique_ptr<CtVolumeMaterial::Instance> CtVolumeMaterial::createInstance(std::unique_ptr<RenderEngine::ITextureView> texture_view, Scene::Scene* scene, uint32_t id)
    {
        using namespace RenderEngine;
        auto& texture = texture_view->getTexture();
        std::unique_ptr<Instance> result = std::make_unique<Instance>();
        std::unordered_map<int32_t, std::unique_ptr<ITextureView>> texture_map;
        texture_map[2] = std::move(texture_view);

        auto on_begin_frame = [material_constants = &result->_material_constants, scene](MaterialInstance::UpdateContext& update_context, uint32_t)
            {
                material_constants->vertex_values.projection = scene->getActiveCamera()->getProjection();
                assert(update_context.getVertexShaderMetaData().push_constants != std::nullopt);
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.projection), sizeof(material_constants->vertex_values.projection));
                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, projection), data_view);
                }
                // For front/back face draw call there is not push constant for the fragment shader
                if (update_context.getFragmentShaderMetaData().push_constants != std::nullopt)
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->fragment_values), sizeof(material_constants->fragment_values));
                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexPushConstants), data_view);
                }
            };

        auto on_draw = [material_constants = &result->_material_constants, scene](MaterialInstance::UpdateContext& update_context, const MeshInstance* mesh)
            {
                auto model = scene->getNodeLookup().findVolumObject(mesh->getId())->getTransformation().calculateTransformation();
                auto view = scene->getActiveCamera()->getView();
                material_constants->vertex_values.model_view = view * model;
                assert(update_context.getVertexShaderMetaData().push_constants != std::nullopt);
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.model_view), sizeof(material_constants->vertex_values.model_view));
                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, model_view), data_view);
                }
            };

        assert((_use_ao == false || _segmentations.size() == 1) && "Only one segmentation is supported with AO calculation");
        result->_material_instance = std::make_unique<VolumeMaterialInstance>(*_material,
                                                                              texture,
                                                                              _segmentations.front().threshold,
                                                                              TextureBindingMap(std::move(texture_map)),
                                                                              VolumeMaterialInstance::CallbackContainer{
                                                                                  .on_frame_begin = std::move(on_begin_frame),
                                                                              .on_draw = std::move(on_draw) },
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