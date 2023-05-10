#define SDL_MAIN_HANDLED
#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

int main()
{
	try
	{
		auto window = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
		bool quit = false;
		SDL_Event event;

		while (!quit)
		{
			SDL_WaitEvent(&event);

			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			}
			window->update();
		}
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
		std::cerr << "Unkown error occurred" << std::endl;
		return -2;
	}
}