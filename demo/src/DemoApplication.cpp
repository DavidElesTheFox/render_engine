#include "DemoApplication.h"

#include <glm/vec4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

#include <render_engine/resources/PushConstantsUpdater.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>

namespace
{

	struct MouseEventData
	{
		glm::vec2 dragging_coordinates{ 0.0f };
		bool is_dragging{ false };
	};

	struct ApplicationContext
	{
		static ApplicationContext& instance()
		{
			static ApplicationContext _instance;
			return _instance;
		}

		Scene::Scene* scene{ nullptr };
		MouseEventData mouse_event_data;
		float mouse_sensitivity = 0.001f;
		GLFWcursorposfun original_cursor_position_callback{ nullptr };
		GLFWmousebuttonfun original_mouse_button_callback{ nullptr };
		GLFWkeyfun originak_key_callback{ nullptr };
	};

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (ApplicationContext::instance().originak_key_callback)
		{
			ApplicationContext::instance().originak_key_callback(window, key, scancode, action, mods);
		}
		auto* scene = ApplicationContext::instance().scene;
		auto* camera = scene->getActiveCamera();

		if (camera == nullptr)
		{
			return;
		}

		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			camera->getTransformation().translate(scene->getSceneSetup().forward);
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			camera->getTransformation().translate(-scene->getSceneSetup().forward);
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			auto left = glm::cross(scene->getSceneSetup().forward, scene->getSceneSetup().up);
			camera->getTransformation().translate(left);
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			auto left = glm::cross(scene->getSceneSetup().forward, scene->getSceneSetup().up);
			camera->getTransformation().translate(-left);
		}


	}

	void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{

		if (ApplicationContext::instance().original_cursor_position_callback)
		{
			ApplicationContext::instance().original_cursor_position_callback(window, xpos, ypos);
		}
		auto* camera = ApplicationContext::instance().scene->getActiveCamera();

		if (camera == nullptr)
		{
			return;
		}
		auto& event_data = ApplicationContext::instance().mouse_event_data;
		if (event_data.is_dragging == false)
		{
			return;
		}
		const glm::vec2 current_coordinates = glm::vec2{ -ypos, xpos };
		const glm::vec2 delta = event_data.dragging_coordinates - current_coordinates;
		
		const glm::quat rotation{ glm::vec3(delta, 0.0f) * ApplicationContext::instance().mouse_sensitivity };

		camera->getTransformation().rotate(rotation);

		event_data.dragging_coordinates = current_coordinates;

	}
	void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if (ApplicationContext::instance().original_mouse_button_callback)
		{
			ApplicationContext::instance().original_mouse_button_callback(window, button, action, mods);
		}
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			if (action == GLFW_PRESS)
			{
				auto& event_data = ApplicationContext::instance().mouse_event_data;
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				
				event_data.dragging_coordinates.x = -ypos;
				event_data.dragging_coordinates.y = xpos;
				event_data.is_dragging = true;
			}
			else if (action == GLFW_RELEASE)
			{
				ApplicationContext::instance().mouse_event_data.is_dragging = false;
			}
		}



	}
}

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
			.global_ubo_update = [material_data = &result->_material_data ](std::vector<UniformBinding>& uniform_bindings, uint32_t frame_number)
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
					material_constants->model_view =  view * model;

					const std::span<const uint8_t> data_view(reinterpret_cast<const uint8_t*>(&material_constants->model_view), sizeof(material_constants->model_view));
					updater.update(offsetof(PushConstants, model_view), data_view);
				}
		},
		0);
	return result;
}

void DemoApplication::init()
{
	using namespace RenderEngine;
	
	createAssets();

	createScene();
	
	initializeRenderers();
	createWindow();

	ApplicationContext::instance().scene = _scene.get();

	ApplicationContext::instance().original_cursor_position_callback = glfwSetCursorPosCallback(_window->getWindowHandle(), cursorPositionCallback);
	ApplicationContext::instance().originak_key_callback = glfwSetKeyCallback(_window->getWindowHandle(), keyCallback);

	ApplicationContext::instance().original_mouse_button_callback = glfwSetMouseButtonCallback(_window->getWindowHandle(), mouseButtonCallback);

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
	_quad_asset = std::make_unique<RenderEngine::Mesh>(_quad_geometry.get(), _nolit_material_new->getMaterial(), 0);
}

void DemoApplication::createNoLitMaterial()
{
	_nolit_material_new = std::make_unique<NoLitMaterial>();
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
		_nolit_material_instance = _nolit_material_new->createInstance(NoLitMaterial::Data{ .color_offset = 0.0f }, _scene.get());
		_quad = std::make_unique<RenderEngine::MeshInstance>(_quad_asset.get(), _nolit_material_instance->getMaterialInstance(), 0);

		Scene::SceneNode::Builder mesh_builder;
		auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh01", _quad.get());
		mesh_builder.add(std::move(mesh_object));
		_scene->addNode(mesh_builder.build("Quad01"));
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
		{ return std::make_unique<ForwardRenderer>(window, swap_chain, has_next); });
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
			float value = _nolit_material_instance->getMaterialData().color_offset;
			ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
			_nolit_material_instance->getMaterialData().color_offset = value;

			auto* mesh_object = _scene->getNodeLookup().findMesh(_quad->getId());
			glm::vec3 rotation = mesh_object->getTransformation().getEulerAngles();
			ImGui::SliderFloat("Rotation z", &rotation.z, -glm::pi<float>(), glm::pi<float>());
			mesh_object->getTransformation().setEulerAngles(rotation);
		});
}
