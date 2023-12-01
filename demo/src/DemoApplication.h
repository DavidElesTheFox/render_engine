#pragma once
#include <data_config.h>
#include <imgui.h>
#include <render_engine/RenderContext.h>
#include <render_engine/Device.h>
#include <render_engine/assets/Geometry.h>
#include <render_engine/resources/Texture.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/resources/UniformBinding.h>
#include <render_engine/assets/Mesh.h>

#include <scene/Scene.h>
#include <scene/SceneRenderManager.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assets/AssetDatabase.h>
#include <assets/NoLitMaterial.h>
#include <ui/AssetBrowserUi.h>


class DemoApplication
{
public:
	void init();

	void run();

	~DemoApplication();

private:
	void createGeometries();
	void createBaseMesh();
	void createBaseMaterials();
	void createScene();
	void loadScene();
	void instantiateMaterials();
	void instantiateMeshes();

	void initializeRenderers();
	void createAssets();

    void onGui();
	void createWindow();

	Assets::AssetDatabase _assets;

	std::unique_ptr<RenderEngine::Window> _window;

	std::unique_ptr<Scene::Scene> _scene;

	std::unique_ptr<Scene::SceneRenderManager> _render_manager;
	std::unique_ptr<Ui::AssetBrowserUi> _asset_browser;
	std::unique_ptr<RenderEngine::Texture> _statue_texture;
	std::unique_ptr<RenderEngine::TextureFactory> _texture_factory;
};
