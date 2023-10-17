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


#include <assets/AssetDatabase.h>
#include <assets/NoLitMaterial.h>



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

	Assets::AssetDatabase _assets;

	std::unique_ptr<RenderEngine::Window> _window;

	std::unique_ptr<Scene::Scene> _scene;
};
