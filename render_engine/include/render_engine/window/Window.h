#pragma once

#include <volk.h>

#include <render_engine/window/SwapChain.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

#include <array>
#include <memory>

#include <GLFW/glfw3.h>
#include <render_engine/Device.h>
#include <render_engine/RenderEngine.h>
#include <render_engine/TransferEngine.h>
struct ImGuiContext;
namespace RenderEngine
{
	class Device;
	class GpuResourceManager;
	class Window
	{
	public:

		static constexpr std::array<const char*, 1> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		Window(Device& device,
			std::unique_ptr<RenderEngine>&& render_engine,
			std::unique_ptr<TransferEngine>&& transfer_engine,
			GLFWwindow* window,
			std::unique_ptr<SwapChain> swap_chain,
			VkQueue present_queue,
			size_t back_buffer_size);
		~Window();

		void update();
		bool isClosed() const { return _closed; }

		void registerRenderers(const std::vector<uint32_t>& renderer_ids);
		template<typename T>
		T& getRendererAs(uint32_t renderer_id)
		{
			return static_cast<T&>(*_renderer_map.at(renderer_id));
		}
		Device& getDevice() { return _device; }
		void enableRenderdocCapture();
		void disableRenderdocCapture();
		GLFWwindow* getWindowHandle() { return _window; }
		RenderEngine& getRenderEngine() { return *_render_engine; }
		TransferEngine& getTransferEngine() { return *_transfer_engine; }
	private:
		struct FrameData
		{
			VkSemaphore image_available_semaphore{VK_NULL_HANDLE};
			VkSemaphore render_finished_semaphore{ VK_NULL_HANDLE };
			VkFence in_flight_fence{ VK_NULL_HANDLE };
		};
		void initSynchronizationObjects();
		void handleEvents();
		void present();
		void present(FrameData& current_frame_data);
		void reinitSwapChain();
		void destroy();
		bool _closed = false;
		VkQueue _present_queue;

		GLFWwindow* _window{ nullptr };
		std::unique_ptr<SwapChain> _swap_chain;

		Device& _device;
		std::unique_ptr<RenderEngine> _render_engine;
		std::unique_ptr<TransferEngine> _transfer_engine;
		std::vector<FrameData> _back_buffer;
		uint32_t _render_queue_family{ 0 };

		std::vector<std::unique_ptr<AbstractRenderer>> _renderers;
		std::unordered_map<uint32_t, AbstractRenderer*> _renderer_map;
		uint32_t _frame_counter{ 0 };
		size_t _back_buffer_size{ 2 };
		void* _renderdoc_api{ nullptr };
	};
}