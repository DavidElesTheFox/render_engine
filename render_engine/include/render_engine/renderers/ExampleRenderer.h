#pragma once
#include <render_engine/CommandPoolFactory.h>
#include <render_engine/renderers/AbstractRenderer.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/window/SwapChain.h>

#include <volk.h>

#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace RenderEngine
{
    class Window;
    class ExampleRenderer : public AbstractRenderer
    {
    public:
        static constexpr uint32_t kRendererId = 0u;
        struct Vertex {
            std::array<float, 2> pos;
            std::array<float, 3> color;
        };

        struct ColorOffset
        {
            float r;
        };

        ExampleRenderer(Window& parent,
                        const RenderTarget& render_target,
                        uint32_t back_buffer_size,
                        bool last_ExampleRenderer);

        void init(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indicies);

        void draw(uint32_t swap_chain_image_index, uint32_t frame_number) override
        {
            draw(_frame_buffers[swap_chain_image_index], frame_number);
        }
        std::vector<VkCommandBuffer> getCommandBuffers(uint32_t frame_number) override
        {
            return { getFrameData(frame_number).command_buffer };
        }

        ColorOffset& getColorOffset() { return _color_offset; }

        ~ExampleRenderer() override;
    private:
        struct FrameData
        {
            VkCommandBuffer command_buffer;
            std::unique_ptr<Buffer> color_offset;
            VkDescriptorSet descriptor_set;
        };
        void createFrameBuffers(const RenderTarget& swap_chain);
        bool createFrameBuffer(const RenderTarget& swap_chain, uint32_t frame_buffer_index);
        void createCommandBuffer();
        FrameData& getFrameData(uint32_t frame_number)
        {
            return _back_buffer[frame_number % _back_buffer.size()];
        }
        void draw(const VkFramebuffer& frame_buffer, uint32_t frame_number);
        void resetFrameBuffers();
        void beforeReinit() override;
        void finalizeReinit(const RenderTarget& swap_chain) override;
        Window& _window;
        VkRenderPass _render_pass;
        VkPipelineLayout _pipeline_layout;
        VkPipeline _pipeline;
        VkDescriptorSetLayout _descriptor_set_layout;
        VkDescriptorPool _descriptor_pool;
        std::vector<VkFramebuffer> _frame_buffers;
        CommandPoolFactory::CommandPool _command_pool;
        std::vector<FrameData> _back_buffer;
        VkRect2D _render_area;

        std::unique_ptr<Buffer> _vertex_buffer;
        std::unique_ptr<Buffer> _index_buffer;
        ColorOffset _color_offset;
    };
}