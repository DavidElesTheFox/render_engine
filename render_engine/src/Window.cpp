#include <render_engine/Window.h>


#include <GLFW/glfw3native.h>


namespace RenderEngine
{
	Window::Window(VkQueue render_queue, std::string_view name)
		: _render_queue{ render_queue }
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(800, 600, name.data(), nullptr, nullptr);
	}
	Window::~Window()
	{
		glfwDestroyWindow(_window);
		_window = nullptr;
	}
}