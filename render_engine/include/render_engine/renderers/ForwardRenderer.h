#pragma once


#include <filesystem>
#include <map>

#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/window/Window.h>
#include <render_engine/containers/BackBuffer.h>

namespace RenderEngine
{
	class Material;
	class Mesh;
	class Technique;
	class Buffer;

	class ForwardRenderer : public AbstractRenderer
	{
	private:
		struct MeshBuffers
		{
			std::unique_ptr<Buffer> vertex_buffer;
			std::unique_ptr<Buffer> index_buffer;
			std::unique_ptr<Buffer> color_buffer;
			std::unique_ptr<Buffer> normal_buffer;
			std::unique_ptr<Buffer> texture_buffer;
		};
		struct MeshGroup
		{
			Technique* technique;
			std::map<Mesh*, MeshBuffers> mesh_buffers;
		};
		struct FrameData
		{
			VkCommandBuffer command_buffer;
		};
		using MeshGroupMap = std::unordered_map<uint32_t, MeshGroup>;
	public:
		static constexpr uint32_t kRendererId = 2u;
		ForwardRenderer(Window& parent,
			const SwapChain& swap_chain,
			bool last_renderer,
			std::vector<Material*> supported_materials);
		~ForwardRenderer() override;
		void addMesh(Mesh* mesh, int32_t priority);
		void draw(uint32_t swap_chain_image_index, uint32_t frame_number) override;
		std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number) override
		{
			return { _back_buffer[frame_number].command_buffer };
		}
	private:
		void createFrameBuffers(const SwapChain& swap_chain);
		void createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index);
		void createCommandPool(uint32_t render_queue_family);
		void createCommandBuffer();

		void resetFrameBuffers();
		void beforeReinit() override;
		void finalizeReinit(const SwapChain& swap_chain) override;
		void destroy();
	private:
		std::map<int32_t, MeshGroupMap> _meshes;
		std::unordered_map<uint32_t, std::unique_ptr<Technique>> _technique_map;
		BackBuffer<FrameData> _back_buffer;

		VkRect2D _render_area;
		Window& _window;

		std::vector<VkFramebuffer> _frame_buffers;
		VkRenderPass _render_pass{ VK_NULL_HANDLE };

		VkCommandPool _command_pool{ VK_NULL_HANDLE };
	};
}
