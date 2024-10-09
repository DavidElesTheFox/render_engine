#pragma once

#include <render_engine/LogicalDevice.h>

#include <render_engine/rendergraph/Node.h>

#include <render_engine/window/SwapChain.h>
#include <render_engine/window/Window.h>

namespace RenderEngine::RenderGraph
{
    class ImageAcquireTask : public CpuNode::ICpuTask
    {
    public:
        constexpr static auto image_available_semaphore_name = "image-available";
        ImageAcquireTask(Window& window,
                         LogicalDevice& logical_device,
                         uint32_t back_buffer_count,
                         std::string image_user_node_name);
        ~ImageAcquireTask();
        void run(CpuNode& self, ExecutionContext& execution_context) final;

        void registerExecutionContext(ExecutionContext& execution_context) override;;

        bool isActive() const final { return true; }
    private:
        class SwapChainImageSelector;

        Window& _window;
        std::unique_ptr<SwapChainImageSelector> _swap_chain_image_selector;
        std::string _image_user_node_name;
    };
}