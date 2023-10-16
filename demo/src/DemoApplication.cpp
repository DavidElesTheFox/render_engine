#include "DemoApplication.h"

#include <glm/vec4.hpp>

#include <render_engine/resources/PushConstantsUpdater.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>

namespace
{
	struct NoLitPushConstants
	{
		glm::mat4 projection;
		glm::mat4 model_view;
		
	};
}
void DemoApplication::init()
{
	using namespace RenderEngine;
	
	createAssets();

	createScene();
	
	initializeRenderers();
	createWindow();
}

void DemoApplication::run()
{
	while (_window->isClosed() == false)
	{
		_window->update();
	}
}

void DemoApplication::createMesh()
{
	_quad_geometry = std::make_unique<RenderEngine::Geometry>();
	_quad_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f });
	_quad_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f });
	_quad_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f });
	_quad_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f });
	_quad_geometry->colors.push_back({ .0f, 0.0f, 0.0f });
	_quad_geometry->colors.push_back({ .0f, 1.0f, 0.0f });
	_quad_geometry->colors.push_back({ .0f, 0.0f, 1.0f });
	_quad_geometry->colors.push_back({ 1.0f, 1.0f, 1.0f });
	_quad_geometry->indexes = {
		0, 1, 2, 2, 3, 0
	};
	_quad = std::make_unique<RenderEngine::Mesh>(_quad_geometry.get(), _nolit_material.get(), 0);
}

void DemoApplication::createNoLitMaterial()
{
	using namespace RenderEngine;

	Shader::MetaData nolit_vertex_meta_data;
	nolit_vertex_meta_data.attributes_stride = 5 * sizeof(float);
	nolit_vertex_meta_data.input_attributes.push_back({ .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 });
	nolit_vertex_meta_data.input_attributes.push_back({ .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 2 * sizeof(float) });
	nolit_vertex_meta_data.push_constants = Shader::MetaData::PushConstants{ .size = sizeof(NoLitPushConstants) };

	Shader::MetaData nolit_frament_meta_data;
	nolit_frament_meta_data.global_uniform_buffers.insert({ 0, Shader::MetaData::Uniforms{.binding = 0, .size = sizeof(float)} });

	Shader nolit_vertex_shader(NOLIT_VERT_SHADER, nolit_vertex_meta_data);
	Shader nolit_fretment_shader(NOLIT_FRAG_SHADER, nolit_frament_meta_data);

	_nolit_material = std::make_unique<Material>(nolit_vertex_shader,
		nolit_fretment_shader,
		Material::CallbackContainer{
			.buffer_update_callback = [&](std::vector<UniformBinding>& uniform_bindings, uint32_t frame_number) {uniform_bindings[0].getBuffer(frame_number).uploadMapped(_nolit_shader_controller.data()); },
			.vertex_buffer_create_callback = [](const Geometry& geometry, const Material& material)
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
			},
		.push_constants_updater = [&](Mesh* mesh, PushConstantsUpdater& updater) {
			NoLitPushConstants data;
			data.model_view = _scene->getActiveCamera()->getView() * 
				_scene->getNodeLookup().findMesh(mesh->getId())->getTransformation().calculateTransformation();

			const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&data.model_view), sizeof(data.model_view));
			updater.update(offsetof(NoLitPushConstants, model_view), data_view);
		},
		.push_constants_global_updater = [&](PushConstantsUpdater& updater) {
			NoLitPushConstants data;
			data.projection = _scene->getActiveCamera()->getProjection();

			const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&data.projection), sizeof(data.projection));
			updater.update(offsetof(NoLitPushConstants, projection), data_view);
		}
		}, 0
	);
}

void DemoApplication::createScene()
{
	{
		Scene::Scene::SceneSetup scene_setup{
			.up = glm::vec3(0.0f, 1.0f, 0.0f),
			.forward = glm::vec3(0.0f, 0.0f, -1.0f)
		};

		_scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
	}
	{
		Scene::SceneNode::Builder camera_builder;
		const std::string camera_name = "Ortho camera";
		const std::string camera_node_name = "Main camera";
		camera_builder.add(Scene::Camera::createOrtho(camera_name,
			{ -1.0f, -1.0f },
			{ 1.0f, 1.0f },
			0.001, 5.0f));

		_scene->addNode(camera_builder.build(camera_node_name));

		auto* active_camera = _scene->getNodeLookup().findCamera(camera_name);
		_scene->setActiveCamera(active_camera);
		active_camera->getTransformation().setPosition({ 0.0f, 0.0f, 2.0f });
	}
	{
		Scene::SceneNode::Builder mesh_builder;
		auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh01", _quad.get());
		mesh_builder.add(std::move(mesh_object));
		_scene->addNode(mesh_builder.build("Quad"));
	}
}

void DemoApplication::initializeRenderers()
{
	using namespace RenderEngine;

	std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
	renderers->registerRenderer(UIRenderer::kRendererId,
		[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
		{ return std::make_unique<UIRenderer>(window, swap_chain, back_buffer_count, previous_renderer == nullptr); });
	renderers->registerRenderer(ForwardRenderer::kRendererId,
		[&](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
		{ return std::make_unique<ForwardRenderer>(window, swap_chain, has_next, std::vector{ _nolit_material.get() }); });
	RenderContext::initialize({ "VK_LAYER_KHRONOS_validation" }, std::move(renderers));
}

void DemoApplication::createAssets()
{
	createNoLitMaterial();
	createMesh();
}

void DemoApplication::createWindow()
{
	using namespace RenderEngine;

	_window = RenderContext::context().getEngine(0).createWindow("Secondary Window", 3);
	_window->registerRenderers({ ForwardRenderer::kRendererId, UIRenderer::kRendererId });
	_window->getRendererAs<ForwardRenderer>(ForwardRenderer::kRendererId).addMesh(_quad.get(), 0);

	_window->getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
		[&] {
			float value = _nolit_shader_controller.color_offset;
			ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
			_nolit_shader_controller.color_offset = value;

			auto* mesh_object = _scene->getNodeLookup().findMesh(_quad->getId());
			glm::vec3 rotation = mesh_object->getTransformation().getEulerAngles();
			ImGui::SliderFloat("Rotation z", &rotation.z, -glm::pi<float>(), glm::pi<float>());
			mesh_object->getTransformation().setEulerAngles(rotation);
		});
}
