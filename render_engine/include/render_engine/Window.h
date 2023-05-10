#pragma once

#include <vulkan/vulkan.h>

#include <string_view>

#include <GLFW/glfw3.h>
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

		bool _closed = false;
		VkQueue _render_queue;

		GLFWwindow* _window{ nullptr };
	};
}