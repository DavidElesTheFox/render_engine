#pragma once

#include <render_engine/SwapChain.h>
#include <render_engine/Drawer.h>
#include <render_engine/GUIDrawer.h>

#include <vulkan/vulkan.h>

#include <string_view>
#include <array>
#include <memory>

#include <GLFW/glfw3.h>
struct ImGuiContext;
namespace RenderEngine
{
	class RenderEngine;
	class Window
	{
	public:

		static constexpr std::array<const char*, 1> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		explicit Window(RenderEngine& engine,
			GLFWwindow* window,
			std::unique_ptr<SwapChain> swap_chain,
			VkQueue render_queue,
			VkQueue present_queue,
			uint32_t render_queue_family);
		~Window();

		void update();
		bool isClosed() const { return _closed; }

		Drawer& registerDrawer(bool as_last_drawer);
		GUIDrawer& registerGUIDrawer();
		RenderEngine& getRenderEngine() { return _engine; }
		uint32_t getRenderQueueFamily() { return _render_queue_family; }
		VkQueue& getRenderQueue() { return _render_queue; }
		void enableRenderdocCapture();
		void disableRenderdocCapture();
	private:
		struct FrameData
		{
			VkSemaphore image_available_semaphore;
			VkSemaphore render_finished_semaphore;
			VkFence in_flight_fence;
		};
		void initSynchronizationObjects();
		void handleEvents();
		void present();
		void present(FrameData& current_frame_data);
		void reinitSwapChain();
		void destroy();
		static constexpr uint32_t kBackBufferSize = 2u;
		bool _closed = false;
		VkQueue _render_queue;
		VkQueue _present_queue;

		GLFWwindow* _window{ nullptr };
		std::unique_ptr<SwapChain> _swap_chain;

		RenderEngine& _engine;
		std::array<FrameData, kBackBufferSize> _back_buffer;
		uint32_t _render_queue_family{ 0 };

		std::vector<std::unique_ptr<Drawer>> _drawers;
		std::vector<std::unique_ptr<GUIDrawer>> _gui_drawers;
		uint32_t _frame_counter{ 0 };
		ImGuiContext* _imgui_context{ nullptr };
		void* _renderdoc_api{ nullptr };
	};
}