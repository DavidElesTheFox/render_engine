#define SDL_MAIN_HANDLED
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <imgui.h>
#include <render_engine/renderers/ExampleRenderer.h>

#define ENABLE_WINDOW_0 true
#define ENABLE_WINDOW_1 true
#define ENABLE_WINDOW_2 true
void run()
{
#if ENABLE_WINDOW_0
	auto window_0 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
	window_0->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_1

	auto window_1 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window");
	window_1->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId, RenderEngine::UIRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_2
	auto window_2 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window");
	window_2->registerRenderers({ RenderEngine::UIRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_0
	auto& example_renderer_0 = window_0->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
#endif
#if ENABLE_WINDOW_1

	auto& ui_renderer_1 = window_1->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
	auto& example_renderer_1 = window_1->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
#endif
#if ENABLE_WINDOW_2
	auto& ui_renderer_2 = window_2->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
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
	bool has_open_window = true;
	while (has_open_window)
	{
		has_open_window = false;
#if ENABLE_WINDOW_0
		window_0->update();
		has_open_window |= window_0->isClosed() == false;
#endif
#if ENABLE_WINDOW_1
		window_1->update();
		has_open_window |= window_1->isClosed() == false;
#endif
#if ENABLE_WINDOW_2
		window_2->update();
		has_open_window |= window_2->isClosed() == false;
#endif
	}
}

int main()
{
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