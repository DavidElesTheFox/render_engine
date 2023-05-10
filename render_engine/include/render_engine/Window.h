#pragma once

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <string_view>

namespace RenderEngine
{
	class Window
	{
	public:
		explicit Window(VkQueue render_queue, std::string_view name);
		~Window();
	private:

		GLFWwindow* _window{ nullptr };
		VkQueue _render_queue;
		VkSurfaceKHR surface;
	};
}