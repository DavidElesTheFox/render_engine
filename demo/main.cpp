#define SDL_MAIN_HANDLED
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>
#include <imgui.h>
void run()
{

	auto window = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
	auto window_2 = RenderEngine::RenderContext::context().getEngine(0).createWindow("Secondary Window");
	auto& drawer = window->registerDrawer(true);
	auto& drawer_3 = window_2->registerGUIDrawer();
	auto& drawer_2 = window_2->registerDrawer(false);
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