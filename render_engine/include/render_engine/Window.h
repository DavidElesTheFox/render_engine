#pragma once

#include <vulkan/vulkan.h>

#include <string_view>
#include <SDL.h>
namespace RenderEngine
{
	class Window
	{
	public:
		explicit Window(VkQueue render_queue, std::string_view name);
		~Window();

		void update();
		bool isClosed() const { return _closed; }

	private:
		void handleEvents();
		void present();

		bool _closed = false;
		VkQueue _render_queue;
		SDL_Window* _window{ nullptr };
		SDL_Renderer* _sdl_renderer{ nullptr };
		SDL_Surface* _sdl_image{ nullptr };
		SDL_Texture* _sdl_texture{ nullptr };

	};
}