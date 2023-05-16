#define SDL_MAIN_HANDLED
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <imgui.h>
void run()
{

	auto window = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
	window->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId });

	auto window_2 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window");
	window_2->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId, RenderEngine::UIRenderer::kRendererId });

	auto& drawer = window->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
	auto& drawer_3 = window_2->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
	auto& drawer_2 = window_2->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
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
		drawer.init(vertices, indices);
		drawer_2.init(vertices, indices);
		drawer_2.getColorOffset().r = 1.f;
		drawer_3.setOnGui([&]
			{
				float value = drawer_2.getColorOffset().r;
				ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
				drawer_2.getColorOffset().r = value;

			});
	}
	while (window_2->isClosed() == false)
	{
		window->update();
		window_2->update();
	}
}

int main()
{
	{
		using namespace RenderEngine;
		std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
		renderers->registerRenderer(UIRenderer::kRendererId,
			[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, bool) -> std::unique_ptr<AbstractRenderer>
			{return std::make_unique<UIRenderer>(window, swap_chain, back_buffer_count); });

		renderers->registerRenderer(ExampleRenderer::kRendererId,
			[](auto& window, const auto& swap_chain, uint32_t back_buffer_count, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
			{return std::make_unique<ExampleRenderer>(window, swap_chain, back_buffer_count, last_renderer); });
		RenderContext::initialize({ "VK_LAYER_KHRONOS_validation" }, std::move(renderers));
	}

	try
	{
		run();
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