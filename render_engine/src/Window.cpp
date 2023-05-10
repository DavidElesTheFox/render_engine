#include <render_engine/Window.h>
#include <data_config.h>
#include <stdexcept>
namespace RenderEngine
{
	Window::Window(VkQueue render_queue, std::string_view name)
		: _render_queue{ render_queue }
	{
		_window = SDL_CreateWindow(name.data(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
		_sdl_renderer = SDL_CreateRenderer(_window, -1, 0);

		_sdl_image = SDL_LoadBMP(EMPTY_SCREEN_IMAGE_PATH);
		if (_sdl_image == nullptr)
		{
			throw std::runtime_error(std::string("Cannot Load empty screen. Error: ") + SDL_GetError());
		}

		_sdl_texture = SDL_CreateTextureFromSurface(_sdl_renderer, _sdl_image);
	}

	void Window::update()
	{
		SDL_RenderCopy(_sdl_renderer, _sdl_texture, NULL, NULL);
		SDL_RenderPresent(_sdl_renderer);
	}


	Window::~Window()
	{
		SDL_DestroyTexture(_sdl_texture);
		SDL_FreeSurface(_sdl_image);
		SDL_DestroyRenderer(_sdl_renderer);
		SDL_DestroyWindow(_window);
	}
}