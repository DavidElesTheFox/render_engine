#define SDL_MAIN_HANDLED
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

void run()
{
	auto window = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
	auto& drawer = window->registerDrawer();
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

	}
	while (window->isClosed() == false)
	{
		window->update();
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