#pragma once

#include <render_engine/SwapChain.h>

#include <vulkan/vulkan.h>

#include <string_view>
#include <array>
#include <memory>

#include <GLFW/glfw3.h>
namespace RenderEngine
{
	class Window
	{
	public:

		static constexpr std::array<const char*, 1> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		explicit Window(GLFWwindow* window, std::unique_ptr<SwapChain> swap_chain, VkQueue render_queue);
		~Window();

		void update();
		bool isClosed() const { return _closed; }

	private:
		void handleEvents();

		bool _closed = false;
		VkQueue _render_queue;

		GLFWwindow* _window{ nullptr };
		std::unique_ptr<SwapChain> _swap_chain;
	};
}