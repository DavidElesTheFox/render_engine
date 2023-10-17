#include "DemoApplication.h"

#include <glm/vec4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

#include <render_engine/resources/PushConstantsUpdater.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>

#include <ApplicationContext.h>


void DemoApplication::init()
{
	using namespace RenderEngine;
	
	createAssets();

	createScene();
	
	initializeRenderers();
	createWindow();

	ApplicationContext::instance().init(_scene.get(), _window->getWindowHandle());
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
	auto* nolit_material = _assets.getBaseMaterialAs<Assets::NoLitMaterial>("NoLit");
	auto quad_geometry = std::make_unique<RenderEngine::Geometry>();
	quad_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f });
	quad_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f });
	quad_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f });
	quad_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f });
	quad_geometry->colors.push_back({ .0f, 0.0f, 0.0f });
	quad_geometry->colors.push_back({ .0f, 1.0f, 0.0f });
	quad_geometry->colors.push_back({ .0f, 0.0f, 1.0f });
	quad_geometry->colors.push_back({ 1.0f, 1.0f, 1.0f });
	quad_geometry->indexes = {
		0, 1, 2, 2, 3, 0
	};
	auto quad_asset = std::make_unique<RenderEngine::Mesh>(quad_geometry.get(), nolit_material->getMaterial(), 0);
	_assets.addGeometry("quad", std::move(quad_geometry));
	_assets.addBaseMesh("quad", std::move(quad_asset));

}

void DemoApplication::createNoLitMaterial()
{
	_assets.addBaseMaterial("NoLit", std::make_unique<Assets::NoLitMaterial>());
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
		constexpr float fov_degrees = 60.0f;
		camera_builder.add(Scene::Camera::createPerspective(camera_name, glm::radians(fov_degrees), 800.0f, 600.0f, 0.001, 10.0f));

		_scene->addNode(camera_builder.build(camera_node_name));

		auto* active_camera = _scene->getNodeLookup().findCamera(camera_name);
		_scene->setActiveCamera(active_camera);
		active_camera->getTransformation().setPosition({ 0.0f, 0.0f, 2.0f });
	}
	{
		auto nolit_material = _assets.getBaseMaterialAs<Assets::NoLitMaterial>("NoLit");
		auto nolit_material_instance = nolit_material->createInstance(Assets::NoLitMaterial::Data{ .color_offset = 0.0f }, _scene.get());
		auto quad_mesh = _assets.getBaseMesh("quad");
		auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh, nolit_material_instance->getMaterialInstance(), 0);
		{
			Scene::SceneNode::Builder mesh_builder;
			auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh01", quad.get());
			mesh_builder.add(std::move(mesh_object));
			_scene->addNode(mesh_builder.build("Quad01"));
		}
		_assets.addMaterialInstance("NoLit", std::move(nolit_material_instance));
		_assets.addMeshInstance("quad", std::move(quad));
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
	_window->getRendererAs<ForwardRenderer>(ForwardRenderer::kRendererId).addMesh(_assets.getMeshInstance("quad"), 0);

	_window->getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
		[&] {
			auto* material = _assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit");
			float value = material->getMaterialData().color_offset;
			ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
			material->getMaterialData().color_offset = value;

			auto* mesh_object = _scene->getNodeLookup().findMesh((_assets.getMeshInstance("quad")->getId()));
			glm::vec3 rotation = mesh_object->getTransformation().getEulerAngles();
			ImGui::SliderFloat("Rotation z", &rotation.z, -glm::pi<float>(), glm::pi<float>());
			mesh_object->getTransformation().setEulerAngles(rotation);
		});
}
