#pragma once
#include <render_engine/SwapChain.h>
#include <render_engine/Buffer.h>
#include <render_engine/AbstractRenderer.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <cassert>
#include <memory>
#include <string>
#include <array>
#include <functional>
struct ImGuiContext;
namespace RenderEngine
{
	class Window;

	class UIRenderer : public AbstractRenderer
	{
	public:
		static constexpr uint32_t kRendererId = 1u;

		UIRenderer(Window& parent,
			const SwapChain& swap_chain,
			uint32_t back_buffer_size);

		void draw(uint32_t swap_chain_image_index, uint32_t frame_number) override
		{
			draw(_frame_buffers[swap_chain_image_index], frame_number);
		}

		std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number) override
		{
			return { getFrameData(frame_number).command_buffer };
		}

		void setOnGui(std::function<void()> on_gui) { _on_gui = on_gui; }

		~UIRenderer();
	private:
		struct FrameData
		{
			VkCommandBuffer command_buffer;
			std::unique_ptr<Buffer> color_offset;
			VkDescriptorSet descriptor_set;
		};
		FrameData& getFrameData(uint32_t frame_number)
		{
			return _back_buffer[frame_number % _back_buffer.size()];
		}
		void createFrameBuffers(const SwapChain& swap_chain);
		bool createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index);
		void createCommandPool(uint32_t render_queue_family);
		void createCommandBuffer();
		void draw(const VkFramebuffer& frame_buffer, uint32_t frame_number);
		void resetFrameBuffers();
		void beforeReinit() override;
		void finalizeReinit(const SwapChain& swap_chain) override;
		Window& _window;
		ImGuiContext* _imgui_context{ nullptr };

		VkRenderPass _render_pass;
		VkDescriptorPool _descriptor_pool;
		std::vector<VkFramebuffer> _frame_buffers;
		VkCommandPool _command_pool;
		std::vector<FrameData> _back_buffer;
		VkRect2D _render_area;

		std::function<void()> _on_gui;

	};
}