#define SDL_MAIN_HANDLED
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <imgui.h>
void run()
{

	auto window = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
	auto window_2 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window");
	auto window_3 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window");
	auto& drawer = window->registerDrawer();
	auto& drawer_2 = window_2->registerDrawer();
	auto& drawer_3 = window_3->registerGUIDrawer();
	{
		const std::vector<RenderEngine::Drawer::Vertex> vertices = {
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
		drawer_2.getColorOffset().r = 150.f;
		drawer_3.setOnGui([]
			{
				ImGui::ShowDemoWindow();

			});
	}
	while (window_3->isClosed() == false)
	{
		window->update();
		window_2->update();
		window_3->update();
	}
}

int main()
{

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