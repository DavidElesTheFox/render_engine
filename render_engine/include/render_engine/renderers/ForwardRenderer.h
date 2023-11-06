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
	class MeshInstance;
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
			std::unique_ptr<Technique> technique;
			std::vector<const MeshInstance*> mesh_instances;
		};
		struct FrameData
		{
			VkCommandBuffer command_buffer;
		};
		using MaterialMeshGroupMap = std::unordered_map<uint32_t, MeshGroup>;
	public:
		static constexpr uint32_t kRendererId = 2u;
		ForwardRenderer(Window& parent,
			const SwapChain& swap_chain,
			bool last_renderer);
		~ForwardRenderer() override;
		void addMesh(const MeshInstance* mesh_instance, int32_t priority);
		void draw(uint32_t swap_chain_image_index, uint32_t frame_number) override;
		std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number) override
		{
			return { _back_buffer[frame_number].command_buffer };
		}
	private:
		void createFrameBuffers(const SwapChain& swap_chain);
		void createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index);
		void createCommandBuffer();

		void resetFrameBuffers();
		void beforeReinit() override;
		void finalizeReinit(const SwapChain& swap_chain) override;
		void destroy();
	private:
		std::map<int32_t, MaterialMeshGroupMap> _meshes;
		std::map<const Mesh*, MeshBuffers> _mesh_buffers;

		BackBuffer<FrameData> _back_buffer;

		VkRect2D _render_area;
		Window& _window;

		std::vector<VkFramebuffer> _frame_buffers;
		VkRenderPass _render_pass{ VK_NULL_HANDLE };

		CommandPoolFactory::CommandPool _command_pool{};
	};
}
