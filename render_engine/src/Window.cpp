#include <render_engine/Window.h>
#include <data_config.h>
#include <stdexcept>
namespace RenderEngine
{
	Window::Window(VkQueue render_queue, std::string_view name)
		: _render_queue{ render_queue }
	{
		constexpr auto width = 600;
		constexpr auto height = 600;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);
	}

	void Window::update()
	{
		if (_closed)
		{
			return;
		}
		handleEvents();
	}


	Window::~Window()
	{
		glfwDestroyWindow(_window);
	}

	void Window::handleEvents()
	{
		glfwPollEvents();
		_closed = glfwWindowShouldClose(_window);
	}
}