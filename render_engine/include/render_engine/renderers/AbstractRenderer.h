#pragma once

#include <cstdint>
#include <vector>
#include <volk.h>

namespace RenderEngine
{
	class SwapChain;
	class AbstractRenderer
	{
	public:
		class ReinitializationCommand
		{
		public:
			explicit ReinitializationCommand(AbstractRenderer& renderer);

			void finish(const SwapChain& swap_chain);
			~ReinitializationCommand();
		private:
			AbstractRenderer& _renderer;
			bool _finished = false;
		};
		friend class ReinitializationCommand;

		virtual ~AbstractRenderer() = default;

		virtual void draw(uint32_t swap_chain_image_index, uint32_t frame_number) = 0;
		virtual std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number) = 0;
		[[nodiscard]]
		ReinitializationCommand reinit();
	private:
		virtual void beforeReinit() = 0;
		virtual void finalizeReinit(const SwapChain& swap_chain) = 0;

	};
}