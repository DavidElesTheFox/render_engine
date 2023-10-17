#pragma once
#include <data_config.h>
#include <imgui.h>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/resources/UniformBinding.h>
#include <render_engine/assets/Mesh.h>

#include <scene/Scene.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class NoLitMaterial
{
public:
	struct Data
	{
		float color_offset{ 0.0f };
	};

	class PushConstants
	{
		friend class NoLitMaterial;
	public:
	private:
		glm::mat4 projection;
		glm::mat4 model_view;
	};

	class Instance
	{
		friend class NoLitMaterial;
	public:
		Instance() = default;
		Data& getMaterialData() { return _material_data; }
		const Data& getMaterialData() const { return _material_data; }
		PushConstants& getMaterialConstants() { return _material_constants; }
		const PushConstants& getMaterialConstants() const { return _material_constants; }
		RenderEngine::MaterialInstance* getMaterialInstance() { return _material_instance.get(); }
	private:
		Data _material_data{};
		PushConstants _material_constants;
		std::unique_ptr<RenderEngine::MaterialInstance> _material_instance;
	};


	NoLitMaterial();
	
	std::unique_ptr<Instance> createInstance(Data data, Scene::Scene* scene);

	RenderEngine::Material* getMaterial() { return _material.get(); }
private:
	std::unique_ptr<RenderEngine::Material> _material;
};

class DemoApplication
{
public:
	void init();

	void run();

private:

	void createMesh();
	void createNoLitMaterial();
	void createScene();

	void initializeRenderers();
	void createAssets();

	void createWindow();

	std::unique_ptr<RenderEngine::Geometry> _quad_geometry;
	std::unique_ptr<RenderEngine::Mesh> _quad_asset;

	std::unique_ptr<NoLitMaterial> _nolit_material_new;
	std::unique_ptr<NoLitMaterial::Instance> _nolit_material_instance;

	std::unique_ptr<RenderEngine::MeshInstance> _quad;

	std::unique_ptr<RenderEngine::Window> _window;

	std::unique_ptr<Scene::Scene> _scene;
};
