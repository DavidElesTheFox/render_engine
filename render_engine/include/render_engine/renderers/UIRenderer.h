#pragma once

#include <render_engine/CommandPoolFactory.h>

#include <render_engine/window/SwapChain.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/renderers/AbstractRenderer.h>

#include <volk.h>

#include <vector>
#include <cassert>
#include <memory>
#include <string>
#include <array>
#include <functional>
#include <set>
struct ImGuiContext;
namespace RenderEngine
{
	class Window;

	class UIRenderer : public AbstractRenderer
	{
		static std::set<ImGuiContext*> kInvalidContexts;
	public:
		static constexpr uint32_t kRendererId = 1u;

		UIRenderer(Window& parent,
			const RenderTarget& render_target,
			uint32_t back_buffer_size,
			bool first_renderer);

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

		static void registerDeletedContext(ImGuiContext* context);
		static bool isValidContext(ImGuiContext* context);

		void createFrameBuffers(const RenderTarget&);
		bool createFrameBuffer(const RenderTarget& render_target, uint32_t frame_buffer_index);
		void createCommandBuffer();
		void draw(const VkFramebuffer& frame_buffer, uint32_t frame_number);
		void resetFrameBuffers();
		void beforeReinit() override;
		void finalizeReinit(const RenderTarget& render_target) override;
		Window& _window;
		ImGuiContext* _imgui_context{ nullptr };
		ImGuiContext* _imgui_context_during_init{ nullptr };

		VkRenderPass _render_pass;
		VkDescriptorPool _descriptor_pool;
		std::vector<VkFramebuffer> _frame_buffers;
		CommandPoolFactory::CommandPool _command_pool;
		std::vector<FrameData> _back_buffer;
		VkRect2D _render_area;

		std::function<void()> _on_gui;

	};
}