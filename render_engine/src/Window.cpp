#include <render_engine/Window.h>

namespace RenderEngine
{
	Window::Window(VkQueue render_queue, std::string_view name)
		: _render_queue{ render_queue }
	{
		_window = SDL_CreateWindow(name.data(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	}
	Window::~Window()
	{
	}
}