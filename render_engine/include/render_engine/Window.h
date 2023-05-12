#pragma once

#include <render_engine/SwapChain.h>
#include <render_engine/Drawer.h>

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
		explicit Window(VkDevice logical_device,
			GLFWwindow* window,
			std::unique_ptr<SwapChain> swap_chain,
			VkQueue render_queue,
			VkQueue present_queue,
			uint32_t render_queue_family);
		~Window();

		void update();
		bool isClosed() const { return _closed; }

		void registerDrawer();

	private:
		void initSynchronizationObjects();
		void handleEvents();
		void present();

		bool _closed = false;
		VkQueue _render_queue;
		VkQueue _present_queue;

		GLFWwindow* _window{ nullptr };
		std::unique_ptr<SwapChain> _swap_chain;
		VkDevice _logical_device;
		VkSemaphore _image_available_semaphore;
		VkSemaphore _render_finished_semaphore;
		VkFence _in_flight_fence;
		uint32_t _render_queue_family{ 0 };

		std::vector<std::unique_ptr<Drawer>> _drawers;
	};
}