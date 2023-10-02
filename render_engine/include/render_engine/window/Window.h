#pragma once

#include <render_engine/window/SwapChain.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

#include <volk.h>

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

		void registerRenderers(const std::vector<uint32_t>& renderer_ids);
		template<typename T>
		T& getRendererAs(uint32_t renderer_id)
		{
			return static_cast<T&>(*_renderer_map.at(renderer_id));
		}
		RenderEngine& getRenderEngine() { return _engine; }
		uint32_t getRenderQueueFamily() { return _render_queue_family; }
		VkQueue& getRenderQueue() { return _render_queue; }
		void enableRenderdocCapture();
		void disableRenderdocCapture();
		GLFWwindow* getWindowHandle() { return _window; }
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

		std::vector<std::unique_ptr<AbstractRenderer>> _renderers;
		std::unordered_map<uint32_t, AbstractRenderer*> _renderer_map;
		uint32_t _frame_counter{ 0 };
		void* _renderdoc_api{ nullptr };
	};
}