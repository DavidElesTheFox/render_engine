#pragma once

#include <cstdint>
#include <vector>
#include <volk.h>

namespace RenderEngine
{

    class RenderTarget;
    class AbstractRenderer
    {
    public:
        class ReinitializationCommand
        {
        public:
            explicit ReinitializationCommand(AbstractRenderer& renderer);
            ReinitializationCommand(const ReinitializationCommand&) = delete;
            ReinitializationCommand& operator=(const ReinitializationCommand&) = delete;

            ReinitializationCommand(ReinitializationCommand&& o)
                : _renderer(o._renderer)
            {
                o._finished = true;
            }
            ReinitializationCommand& operator=(ReinitializationCommand&&) = delete;


            void finish(const RenderTarget& render_target);
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
        virtual void finalizeReinit(const RenderTarget& render_target) = 0;

    };
}