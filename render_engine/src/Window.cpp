#include <render_engine/Window.h>

#include <data_config.h>

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <limits>
namespace
{

}

namespace RenderEngine
{
	Window::Window(GLFWwindow* window, std::unique_ptr<SwapChain> swap_chain, VkQueue render_queue)
		: _render_queue{ render_queue }
		, _window(window)
		, _swap_chain(std::move(swap_chain))
	{

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
		_swap_chain.reset();
		glfwDestroyWindow(_window);
	}

	void Window::handleEvents()
	{
		glfwPollEvents();
		_closed = glfwWindowShouldClose(_window);
	}
}