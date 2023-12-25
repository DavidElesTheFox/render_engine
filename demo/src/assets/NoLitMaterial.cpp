#include <assets/NoLitMaterial.h>

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

    NoLitMaterial::NoLitMaterial(uint32_t id)
    {
        using namespace RenderEngine;

        Shader::MetaData nolit_vertex_meta_data;
        nolit_vertex_meta_data.attributes_stride = 3 * sizeof(float);
        nolit_vertex_meta_data.input_attributes.push_back({ .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 });
        nolit_vertex_meta_data.push_constants = Shader::MetaData::PushConstants{ .size = sizeof(VertexPushConstants),.offset = 0 };

        Shader::MetaData nolit_frament_meta_data;
        nolit_frament_meta_data.push_constants = Shader::MetaData::PushConstants{ .size = sizeof(FragmentPushConstants), .offset = sizeof(VertexPushConstants) };
        std::filesystem::path base_path = SHADER_BASE;
        auto nolit_vertex_shader = std::make_unique<Shader>(base_path / "nolit.vert.spv", nolit_vertex_meta_data);
        auto nolit_fretment_shader = std::make_unique<Shader>(base_path / "nolit.frag.spv", nolit_frament_meta_data);

        _material = std::make_unique<Material>(std::move(nolit_vertex_shader),
                                               std::move(nolit_fretment_shader),
                                               Material::CallbackContainer{
                                                   .create_vertex_buffer = [](const Geometry& geometry, const Material& material)
                                                   {
                                                       std::vector<float> vertex_buffer_data;
                                                       vertex_buffer_data.reserve(geometry.positions.size() * 3);
                                                       for (uint32_t i = 0; i < geometry.positions.size(); ++i)
                                                       {
                                                           vertex_buffer_data.push_back(geometry.positions[i].x);
                                                           vertex_buffer_data.push_back(geometry.positions[i].y);
                                                           vertex_buffer_data.push_back(geometry.positions[i].z);
                                                       }
                                                       auto begin = reinterpret_cast<uint8_t*>(vertex_buffer_data.data());
                                                       std::vector<uint8_t> vertex_buffer(begin, begin + vertex_buffer_data.size() * sizeof(float));
                                                       return vertex_buffer;
                                                   }
                                               }, id);
    }
    std::unique_ptr<NoLitMaterial::Instance> NoLitMaterial::createInstance(glm::vec3 instance_color, Scene::Scene* scene, uint32_t id)
    {
        using namespace RenderEngine;
        std::unique_ptr<Instance> result = std::make_unique<Instance>();
        result->_material_constants.fragment_values.instance_color = instance_color;
        auto on_begin_frame = [material_constants = &result->_material_constants, scene](MaterialInstance::UpdateContext& update_context, uint32_t frame_count)
            {
                material_constants->vertex_values.projection = scene->getActiveCamera()->getProjection();
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->vertex_values.projection), sizeof(material_constants->vertex_values.projection));
                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_VERTEX_BIT, offsetof(VertexPushConstants, projection), data_view);
                }
                {
                    const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->fragment_values.instance_color), sizeof(material_constants->fragment_values.instance_color));

                    update_context.getPushConstantUpdater().update(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexPushConstants) + offsetof(FragmentPushConstants, instance_color), data_view);
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
                                                                        TextureBindingMap{},
                                                                        MaterialInstance::CallbackContainer{
                                                                            .on_frame_begin = std::move(on_begin_frame),
                                                                            .on_draw = std::move(on_draw) },
                                                                            id);
        return result;
    }
}