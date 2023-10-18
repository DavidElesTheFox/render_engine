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
	createScene();

	createAssets();

	instantiateMaterials();
	instantiateMeshes();

	loadScene();
	
	initializeRenderers();
	createWindow();

	ApplicationContext::instance().init(_scene.get(), _window->getWindowHandle());

	_render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), *_window);

	_render_manager->registerMeshesForRender();

	_asset_browser = std::make_unique<Ui::AssetBrowserUi>(_assets, *_scene);
}

void DemoApplication::run()
{
	while (_window->isClosed() == false)
	{
		_window->update();

	}
}

void DemoApplication::createGeometries()
{
	auto quad_geometry = std::make_unique<RenderEngine::Geometry>();
	quad_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f });
	quad_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f });
	quad_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f });
	quad_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f });
	quad_geometry->indexes = {
		0, 1, 2, 2, 3, 0
	};
	_assets.addGeometry("quad", std::move(quad_geometry));
}

void DemoApplication::createBaseMesh()
{
	auto* nolit_material = _assets.getBaseMaterial<Assets::NoLitMaterial>();
	auto* quad_geometry = _assets.getGeometry("quad");

	auto quad_asset = std::make_unique<RenderEngine::Mesh>(quad_geometry, nolit_material->getMaterial(), 0);
	_assets.addBaseMesh("quad", std::move(quad_asset));

}

void DemoApplication::createBaseMaterials()
{
	_assets.addBaseMaterial(std::make_unique<Assets::NoLitMaterial>());
}

void DemoApplication::createScene()
{
	Scene::Scene::SceneSetup scene_setup{
		.up = glm::vec3(0.0f, 1.0f, 0.0f),
		.forward = glm::vec3(0.0f, 0.0f, -1.0f)
	};

	_scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
}

void DemoApplication::loadScene()
{

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
		Scene::SceneNode::Builder mesh_builder;
		auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh00", _assets.getMeshInstance("white_quad00"));
		mesh_object->getTransformation().setPosition(glm::vec3{ 1.0f, -1.0f, 0.0f });
		mesh_builder.add(std::move(mesh_object));
		_scene->addNode(mesh_builder.build("Quad00"));
	}
	{
		Scene::SceneNode::Builder mesh_builder;
		auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh01", _assets.getMeshInstance("white_quad01"));
		mesh_object->getTransformation().setPosition(glm::vec3{ 1.0f, 1.0f, 0.0f });
		mesh_builder.add(std::move(mesh_object));
		_scene->addNode(mesh_builder.build("Quad01"));
	}
	{
		Scene::SceneNode::Builder mesh_builder;
		auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh02", _assets.getMeshInstance("red_quad00"));
		mesh_object->getTransformation().setPosition(glm::vec3{ -1.0f, 1.0f, 0.0f });
		mesh_builder.add(std::move(mesh_object));
		_scene->addNode(mesh_builder.build("Quad02"));
	}
	{
		Scene::SceneNode::Builder mesh_builder;
		auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh03", _assets.getMeshInstance("red_quad01"));
		mesh_object->getTransformation().setPosition(glm::vec3{ -1.0f, -1.0f, 0.0f });
		mesh_builder.add(std::move(mesh_object));
		_scene->addNode(mesh_builder.build("Quad03"));
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
	createGeometries();
	createBaseMaterials();
	createBaseMesh();
}

void DemoApplication::instantiateMaterials()
{
	auto nolit_material = _assets.getBaseMaterial<Assets::NoLitMaterial>();
	_assets.addMaterialInstance("NoLit - white", nolit_material->createInstance(glm::vec3{ 1.0f }, _scene.get(), 0));
	_assets.addMaterialInstance("NoLit - red", nolit_material->createInstance(glm::vec3{ 1.0f, 0.0f, 0.0f }, _scene.get(), 1));
}

void DemoApplication::instantiateMeshes()
{
	
	auto quad_mesh = _assets.getBaseMesh("quad");
	{
		auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
			_assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - white")->getMaterialInstance(),
			0);
		_assets.addMeshInstance("white_quad00", std::move(quad));
	}
	{
		auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
			_assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - white")->getMaterialInstance(),
			1);
		_assets.addMeshInstance("white_quad01", std::move(quad));
	}
	{
		auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
			_assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - red")->getMaterialInstance(),
			2);
		_assets.addMeshInstance("red_quad00", std::move(quad));
	}
	{
		auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
			_assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - red")->getMaterialInstance(),
			3);
		_assets.addMeshInstance("red_quad01", std::move(quad));
	}
}


void DemoApplication::createWindow()
{
	using namespace RenderEngine;

	_window = RenderContext::context().getEngine(0).createWindow("Secondary Window", 3);
	_window->registerRenderers({ ForwardRenderer::kRendererId, UIRenderer::kRendererId });
	
	_window->getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
		[&] {
			_asset_browser->onGui();
		});
}


