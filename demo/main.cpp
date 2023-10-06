#define SDL_MAIN_HANDLED
#include <data_config.h>
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <render_engine/window/Window.h>
#include <render_engine/renderers/ExampleRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

#include <imgui.h>
#include <render_engine/renderers/MainCameraRenderer.h>

#define ENABLE_WINDOW_0 true
#define ENABLE_WINDOW_1 true
#define ENABLE_WINDOW_2 true

class MultiWindowApplication
{
public:
	void init()
	{
		initEngine();
#if ENABLE_WINDOW_0
		_window_0 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window", 3);
		_window_0->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_1

		_window_1 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window", 3);
		_window_1->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId, RenderEngine::UIRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_2
		_window_2 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window", 3);
		_window_2->registerRenderers({ RenderEngine::UIRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_0
		auto& example_renderer_0 = _window_0->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
#endif
#if ENABLE_WINDOW_1

		auto& ui_renderer_1 = _window_1->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
		auto& example_renderer_1 = _window_1->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
#endif
#if ENABLE_WINDOW_2
		auto& ui_renderer_2 = _window_2->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
#endif
		{
			const std::vector<RenderEngine::ExampleRenderer::Vertex> vertices = {
				{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
				{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
				{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
				{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
			};
			const std::vector<uint16_t> indices = {
				0, 1, 2, 2, 3, 0
			};
#if ENABLE_WINDOW_0

			example_renderer_0.init(vertices, indices);
#endif
#if ENABLE_WINDOW_1

			example_renderer_1.init(vertices, indices);
			example_renderer_1.getColorOffset().r = 1.f;
			ui_renderer_1.setOnGui([&]
				{
					float value = example_renderer_1.getColorOffset().r;
					ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
					example_renderer_1.getColorOffset().r = value;

				});
#endif
#if ENABLE_WINDOW_2
			ui_renderer_2.setOnGui([] {ImGui::ShowDemoWindow(); });
#endif
		}
	}
	void run()
	{

		bool has_open_window = true;
		while (has_open_window)
		{
			has_open_window = false;
#if ENABLE_WINDOW_0
			_window_0->update();
			has_open_window |= _window_0->isClosed() == false;
#endif
#if ENABLE_WINDOW_1
			_window_1->update();
			has_open_window |= _window_1->isClosed() == false;
#endif
#if ENABLE_WINDOW_2
			_window_2->update();
			has_open_window |= _window_2->isClosed() == false;
#endif
		}
	}
private:

	void initEngine()
	{
		using namespace RenderEngine;
		std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
		renderers->registerRenderer(UIRenderer::kRendererId,
			[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
			{return std::make_unique<UIRenderer>(window, swap_chain, back_buffer_count, previous_renderer == nullptr); });

		renderers->registerRenderer(ExampleRenderer::kRendererId,
			[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
			{return std::make_unique<ExampleRenderer>(window, swap_chain, back_buffer_count, has_next); });
		RenderContext::initialize({ "VK_LAYER_KHRONOS_validation" }, std::move(renderers));
	}
	std::unique_ptr<RenderEngine::Window> _window_0;
	std::unique_ptr<RenderEngine::Window> _window_1;
	std::unique_ptr<RenderEngine::Window> _window_2;
};


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
	void init()
	{
		using namespace RenderEngine;
		std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
		renderers->registerRenderer(UIRenderer::kRendererId,
			[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
			{return std::make_unique<UIRenderer>(window, swap_chain, back_buffer_count, previous_renderer == nullptr); });

		Shader::MetaData nolit_vertex_meta_data;
		nolit_vertex_meta_data.stride = 6 * sizeof(float);
		nolit_vertex_meta_data.input_attributes.push_back({ .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 });
		nolit_vertex_meta_data.input_attributes.push_back({ .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 3 * sizeof(float) });

		Shader::MetaData nolit_frament_meta_data;
		nolit_frament_meta_data.uniform_buffers.insert({ 0, Shader::MetaData::Uniforms{.binding = 0, .size = sizeof(float)} });

		Shader nolit_vertex_shader(BASE_VERT_SHADER, nolit_vertex_meta_data);
		Shader nolit_fretment_shader(BASE_FRAG_SHADER, nolit_frament_meta_data);
		_nolit_material = std::make_unique<Material>(nolit_vertex_shader,
			nolit_fretment_shader,
			[&](std::vector<UniformBinding>& uniform_bindings, uint32_t frame_number) {uniform_bindings[0].buffer(frame_number).uploadMapped(_nolit_shader_controller.data()); },
			[](const Geometry& geometry, const Material& material)
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
			}, 0
			);


		renderers->registerRenderer(ForwardRenderer::kRendererId,
			[&](auto& window, const auto& swap_chain, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
			{return std::make_unique<ForwardRenderer>(window, swap_chain, has_next, std::vector{ _nolit_material.get() }); });

		RenderContext::initialize({ "VK_LAYER_KHRONOS_validation" }, std::move(renderers));
		createMesh();

		_window = RenderContext::context().getEngine(0).createWindow("Secondary Window", 3);
		_window->registerRenderers({ ForwardRenderer::kRendererId, UIRenderer::kRendererId });
		_window->getRendererAs<ForwardRenderer>(ForwardRenderer::kRendererId).addMesh(_triangle.get(), 0);

		_window->getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
			[&] {
				float value = _nolit_shader_controller.color_offset;
				ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
				_nolit_shader_controller.color_offset = value; });
		_window->enableRenderdocCapture();
	}

	void run()
	{
		while (_window->isClosed() == false)
		{
			_window->update();
		}
	}
private:

	void createMesh()
	{
		_triangle_geometry = std::make_unique<RenderEngine::Geometry>();
		_triangle_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f });
		_triangle_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f });
		_triangle_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f });
		_triangle_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f });
		_triangle_geometry->colors.push_back({ .0f, 0.0f, 0.0f });
		_triangle_geometry->colors.push_back({ .0f, 1.0f, 0.0f });
		_triangle_geometry->colors.push_back({ .0f, 0.0f, 1.0f });
		_triangle_geometry->colors.push_back({ 1.0f, 1.0f, 1.0f });
		_triangle_geometry->indexes = {
			0, 1, 2, 2, 3, 0
		};
		_triangle = std::make_unique<RenderEngine::Mesh>(_triangle_geometry.get(), _nolit_material.get(), 0);
	}
	NoLitShaderController _nolit_shader_controller;
	std::unique_ptr<RenderEngine::Geometry> _triangle_geometry;
	std::unique_ptr<RenderEngine::Mesh> _triangle;
	std::unique_ptr<RenderEngine::Material> _nolit_material;

	std::unique_ptr<RenderEngine::Window> _window;
};
void runMultiWindowApplication()
{
	MultiWindowApplication application;
	application.init();
	application.run();
}

void runDemoApplication()
{
	DemoApplication application;
	application.init();
	application.run();
}

int main()
{

	try
	{
		runDemoApplication();
		RenderEngine::RenderContext::context().reset();
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error occurred: " << exception.what() << std::endl;
		return -1;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << std::endl;
		return -2;
	}
}