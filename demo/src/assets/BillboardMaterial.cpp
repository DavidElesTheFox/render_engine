#include <assets/BillboardMaterial.h>

#include <data_config.h>

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/PushConstantsUpdater.h>
#include <render_engine/resources/UniformBinding.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>
#include <scene/Scene.h>
#include <scene/SceneNodeLookup.h>

#include <demo/resource_config.h>

#include <ApplicationContext.h>
namespace Assets
{

    BillboardMaterial::BillboardMaterial(uint32_t id)
    {
        using namespace RenderEngine;

        Shader::MetaData vertex_meta_data;
        vertex_meta_data.attributes_stride = 5 * sizeof(float);
        vertex_meta_data.input_attributes.push_back({ .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 });
        vertex_meta_data.input_attributes.push_back({ .location = 1, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 3 * sizeof(float) });
        vertex_meta_data.push_constants = Shader::MetaData::PushConstants{ .size = sizeof(VertexPushConstants),.offset = 0, .update_frequency = Shader::MetaData::UpdateFrequency::PerDrawCall };

        Shader::MetaData frament_meta_data;
        frament_meta_data.samplers = { {1, Shader::MetaData::Sampler{.binding = 1, .update_frequency = Shader::MetaData::UpdateFrequency::PerFrame}} };


        std::filesystem::path base_path = SHADER_BASE;
        auto vertex_shader = std::make_unique<Shader>(base_path / "billboard.vert.spv", vertex_meta_data);
        auto fretment_shader = std::make_unique<Shader>(base_path / "billboard.frag.spv", frament_meta_data);

        _material = std::make_unique<Material>(std::move(vertex_shader),
                                               std::move(fretment_shader),
                                               Material::CallbackContainer{
                                                   .create_vertex_buffer = [](const Geometry& geometry, const Material& material)
                                                   {
                                                       std::vector<float> vertex_buffer_data;
                                                       vertex_buffer_data.reserve(geometry.positions.size() * 5);
                                                       for (uint32_t i = 0; i < geometry.positions.size(); ++i)
                                                       {
                                                           vertex_buffer_data.push_back(geometry.positions[i].x);
                                                           vertex_buffer_data.push_back(geometry.positions[i].y);
                                                           vertex_buffer_data.push_back(geometry.positions[i].z);

                                                           vertex_buffer_data.push_back(geometry.uv[i].x);
                                                           vertex_buffer_data.push_back(geometry.uv[i].y);
                                                       }
                                                       auto begin = reinterpret_cast<uint8_t*>(vertex_buffer_data.data());
                                                       std::vector<uint8_t> vertex_buffer(begin, begin + vertex_buffer_data.size() * sizeof(float));
                                                       return vertex_buffer;
                                                   }
                                               }, id);
    }
    std::unique_ptr<BillboardMaterial::Instance> BillboardMaterial::createInstance(std::unique_ptr<RenderEngine::TextureView> texture, Scene::Scene* scene, uint32_t id)
    {
        using namespace RenderEngine;
        std::unique_ptr<Instance> result = std::make_unique<Instance>();
        std::unordered_map<int32_t, std::unique_ptr<ITextureView>> texture_map;
        texture_map[1] = std::move(texture);

        auto on_begin_frame = [material_constants = &result->_material_constants, scene](MaterialInstance::UpdateContext& update_context, uint32_t frame_count)
            {
                material_constants->vertex_values.projection = scene->getActiveCamera()->getProjection();
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.projection), sizeof(material_constants->vertex_values.projection));
                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, projection), data_view);
                }
            };
        auto on_draw = [material_constants = &result->_material_constants, scene](MaterialInstance::UpdateContext& update_context, const MeshInstance* mesh)
            {
                auto model = scene->getNodeLookup().findMesh(mesh->getId())->getTransformation().calculateTransformation();
                auto view = scene->getActiveCamera()->getView();
                material_constants->vertex_values.model_view = view * model;

                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.model_view), sizeof(material_constants->vertex_values.model_view));
                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, model_view), data_view);
                }
            };
        result->_material_instance = std::make_unique<MaterialInstance>(*_material,
                                                                        TextureBindingMap(std::move(texture_map)),
                                                                        MaterialInstance::CallbackContainer{
                                                                            .on_frame_begin = std::move(on_begin_frame),
                                                                            .on_draw = std::move(on_draw) },
                                                                            id);
        return result;
    }
}