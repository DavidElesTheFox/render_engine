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


class DemoApplication
{
private:
	struct NoLitShaderController
	{
		float color_offset{ 0.0f };
		std::span<uint8_t> data()
		{
			uint8_t* ptr = reinterpret_cast<uint8_t*>(&color_offset);
			return std::span(ptr, sizeof(float));
		}
	};

public:
	void init();

	void run();

private:

	void createMesh();
	NoLitShaderController _nolit_shader_controller;
	std::unique_ptr<RenderEngine::Geometry> _triangle_geometry;
	std::unique_ptr<RenderEngine::Mesh> _triangle;
	std::unique_ptr<RenderEngine::Material> _nolit_material;

	std::unique_ptr<RenderEngine::Window> _window;
};
