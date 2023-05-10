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
	private:
		VkQueue _render_queue;
		SDL_Window* _window{ nullptr };
	};
}