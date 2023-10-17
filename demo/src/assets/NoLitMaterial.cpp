#include <assets/NoLitMaterial.h>

#include <data_config.h>

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/UniformBinding.h>
#include <render_engine/resources/PushConstantsUpdater.h>

#include <scene/Scene.h>
#include <scene/Camera.h>
#include <scene/MeshObject.h>
#include <scene/SceneNodeLookup.h>
namespace Assets
{

	NoLitMaterial::NoLitMaterial()
	{
		using namespace RenderEngine;

		Shader::MetaData nolit_vertex_meta_data;
		nolit_vertex_meta_data.attributes_stride = 5 * sizeof(float);
		nolit_vertex_meta_data.input_attributes.push_back({ .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 });
		nolit_vertex_meta_data.input_attributes.push_back({ .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 2 * sizeof(float) });
		nolit_vertex_meta_data.push_constants = Shader::MetaData::PushConstants{ .size = sizeof(PushConstants) };

		Shader::MetaData nolit_frament_meta_data;
		nolit_frament_meta_data.global_uniform_buffers.insert({ 0, Shader::MetaData::Uniforms{.binding = 0, .size = sizeof(float)} });

		Shader nolit_vertex_shader(NOLIT_VERT_SHADER, nolit_vertex_meta_data);
		Shader nolit_fretment_shader(NOLIT_FRAG_SHADER, nolit_frament_meta_data);

		_material = std::make_unique<Material>(nolit_vertex_shader,
			nolit_fretment_shader,
			Material::CallbackContainer{
				.create_vertex_buffer = [](const Geometry& geometry, const Material& material)
				{
					std::vector<float> vertex_buffer_data;
					vertex_buffer_data.reserve(geometry.positions.size() * 5);
					for (uint32_t i = 0; i < geometry.positions.size(); ++i)
					{
						vertex_buffer_data.push_back(geometry.positions[i].x);
						vertex_buffer_data.push_back(geometry.positions[i].y);
						vertex_buffer_data.push_back(geometry.colors[i].r);
						vertex_buffer_data.push_back(geometry.colors[i].g);
						vertex_buffer_data.push_back(geometry.colors[i].b);
					}
					auto begin = reinterpret_cast<uint8_t*>(vertex_buffer_data.data());
					std::vector<uint8_t> vertex_buffer(begin, begin + vertex_buffer_data.size() * sizeof(float));
					return vertex_buffer;
				}
			}, 0
		);
	}
	std::unique_ptr<NoLitMaterial::Instance> NoLitMaterial::createInstance(Data data, Scene::Scene* scene)
	{
		using namespace RenderEngine;
		std::unique_ptr<Instance> result = std::make_unique<Instance>();
		result->_material_data = std::move(data);
		result->_material_instance = std::make_unique<MaterialInstance>(_material.get(),
			MaterialInstance::CallbackContainer{
				.global_ubo_update = [material_data = &result->_material_data](std::vector<UniformBinding>& uniform_bindings, uint32_t frame_number)
				{
					uint8_t* ptr = reinterpret_cast<uint8_t*>(material_data);
					std::span data_span(ptr, sizeof(NoLitMaterial::Data));

					uniform_bindings[0].getBuffer(frame_number).uploadMapped(data_span);
				},
				.global_push_constants_update = [material_constants = &result->_material_constants, scene](PushConstantsUpdater& updater) {
					material_constants->projection = scene->getActiveCamera()->getProjection();

					const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->projection), sizeof(material_constants->projection));
					updater.update(offsetof(PushConstants, projection), data_view);
				},
				.push_constants_updater = [material_constants = &result->_material_constants, scene](MeshInstance* mesh, PushConstantsUpdater& updater) {
						auto model = scene->getNodeLookup().findMesh(mesh->getId())->getTransformation().calculateTransformation();
						auto view = scene->getActiveCamera()->getView();
						material_constants->model_view = view * model;

						const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->model_view), sizeof(material_constants->model_view));
						updater.update(offsetof(PushConstants, model_view), data_view);
					}
			},
			0);
		return result;
	}
}