#include <render_engine/window/Window.h>

#include <render_engine/Device.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/RenderContext.h>

#include <data_config.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <backends/imgui_impl_vulkan.h>
#include <data_config.h>

#ifdef ENABLE_RENDERDOC
#	include <renderdoc_app.h>
#else
struct NullRenderdocApi
{
    void StartFrameCapture(void*, void*) {}
    void EndFrameCapture(void*, void*) {};
};
#	define RENDERDOC_API_1_1_2 NullRenderdocApi
#endif

namespace RenderEngine
{
    Window::Window(Device& device,
                   std::unique_ptr<RenderEngine>&& render_engine,
                   std::unique_ptr<TransferEngine>&& transfer_engine,
                   GLFWwindow* window,
                   std::unique_ptr<SwapChain> swap_chain,
                   VkQueue present_queue,
                   size_t back_buffer_size)
        try : _present_queue{ present_queue }
        , _window(window)
        , _swap_chain(std::move(swap_chain))
        , _device(device)
        , _render_engine(std::move(render_engine))
        , _transfer_engine(std::move(transfer_engine))
        , _back_buffer_size(back_buffer_size)
    {
        _back_buffer.reserve(_back_buffer_size);
        for (uint32_t i = 0; i < _back_buffer_size; ++i)
        {
            _back_buffer.emplace_back(FrameData{
                .synch_present = SynchronizationObject::CreateEmpty(device.getLogicalDevice())
                , .synch_render = SynchronizationObject::CreateWithFence(device.getLogicalDevice(), VK_FENCE_CREATE_SIGNALED_BIT) });
        }
        initSynchronizationObjects();
    }
    catch (const std::runtime_error)
    {
        destroy();
    }

    void Window::update()
    {
        if (_closed)
        {
            destroy();
            return;
        }
        present();
        handleEvents();
    }

    void Window::registerRenderers(const std::vector<uint32_t>& renderer_ids)
    {
        _renderers = RenderContext::context().getRendererFactory().generateRenderers(renderer_ids, *this, _swap_chain->createRenderTarget(), _back_buffer.size());
        for (size_t i = 0; i < renderer_ids.size(); ++i)
        {
            _renderer_map[renderer_ids[i]] = _renderers[i].get();
        }
    }

    void Window::enableRenderdocCapture()
    {
        if (_renderdoc_api != nullptr)
        {
            return;
        }
        _renderdoc_api = RenderContext::context().getRenderdocApi();
    }

    void Window::disableRenderdocCapture()
    {
        _renderdoc_api = nullptr;
    }


    Window::~Window()
    {

        destroy();
    }

    void Window::initSynchronizationObjects()
    {
        for (FrameData& frame_data : _back_buffer)
        {
            frame_data.synch_present.createSemaphore("image-available");
            frame_data.synch_render.createSemaphore("render-finished");

            // the signal is sent from swap chain acquire image
            frame_data.synch_render.addWaitOperation(frame_data.synch_present,
                                                     "image-available",
                                                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

            // the signal is waited in presentInfo during presen
            frame_data.synch_render.addSignalOperation("render-finished",
                                                       VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

        }
    }

    void Window::handleEvents()
    {
        bool should_be_closed = glfwWindowShouldClose(_window);
        if (should_be_closed)
        {
            glfwHideWindow(_window);
            _closed = true;
            return;
        }
        glfwPollEvents();
    }

    void Window::present()
    {
        if (_renderdoc_api) ((RENDERDOC_API_1_1_2*)_renderdoc_api)->StartFrameCapture(NULL, NULL);

        present(_back_buffer[_presented_frame_counter % _back_buffer.size()]);
        if (_renderdoc_api) ((RENDERDOC_API_1_1_2*)_renderdoc_api)->EndFrameCapture(NULL, NULL);
        _frame_counter++;

    }
    void Window::reinitSwapChain()
    {
        auto logical_device = _device.getLogicalDevice();
        vkDeviceWaitIdle(logical_device);
        std::vector<AbstractRenderer::ReinitializationCommand> commands;
        for (auto& drawer : _renderers)
        {
            commands.emplace_back(drawer->reinit());
        }
        _swap_chain->reinit();
        auto render_target = _swap_chain->createRenderTarget();
        for (auto& command : commands)
        {
            command.finish(render_target);
        }
    }
    void Window::destroy()
    {
        if (_window == nullptr)
        {
            return;
        }
        auto logical_device = _device.getLogicalDevice();
        vkDeviceWaitIdle(logical_device);
        _back_buffer.clear();
        _swap_chain.reset();
        _renderers.clear();

        _render_engine.reset();
        _transfer_engine.reset();
        glfwDestroyWindow(_window);

        _window = nullptr;
        _closed = true;
    }
    void Window::present(FrameData& frame_data)
    {
        if (glfwGetWindowAttrib(_window, GLFW_ICONIFIED) == GLFW_TRUE)
        {
            return;
        }
        if (_renderers.empty())
        {
            return;
        }
        auto renderers = _renderers | std::views::transform([](const auto& ptr) { return ptr.get(); });
        auto logical_device = _device.getLogicalDevice();
        if (_swap_chain_image_index == std::nullopt)
        {
            uint32_t image_index = 0;
            vkWaitForFences(logical_device, 1, frame_data.synch_render.getDefaultOperations().getFence(), VK_TRUE, UINT64_MAX);
            auto call_result = vkAcquireNextImageKHR(logical_device,
                                                     _swap_chain->getDetails().swap_chain,
                                                     UINT64_MAX,
                                                     frame_data.synch_present.getPrimitives().getSemaphore("image-available"),
                                                     VK_NULL_HANDLE,
                                                     &image_index);
            switch (call_result)
            {
                case VK_ERROR_OUT_OF_DATE_KHR:
                case VK_SUBOPTIMAL_KHR:
                    reinitSwapChain();
                    return;
                case VK_SUCCESS:
                    break;
                default:
                    throw std::runtime_error("Failed to query swap chain image");
            }
            _swap_chain_image_index = image_index;
        }
        vkResetFences(logical_device, 1, frame_data.synch_render.getDefaultOperations().getFence());
        _render_engine->onFrameBegin(renderers, *_swap_chain_image_index);


        bool draw_call_recorded = _render_engine->render(frame_data.synch_render.getDefaultOperations(),
                                                         renderers,
                                                         *_swap_chain_image_index);
        if (draw_call_recorded)
        {
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;


            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &frame_data.synch_render.getPrimitives().getSemaphore("render-finished");
            VkSwapchainKHR swapChains[] = { _swap_chain->getDetails().swap_chain };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;

            presentInfo.pImageIndices = &*_swap_chain_image_index;

            vkQueuePresentKHR(_present_queue, &presentInfo);
            _swap_chain_image_index = std::nullopt;
            _presented_frame_counter++;
        }

    }
    void Window::registerTunnel(WindowTunnel& tunnel)
    {
        if (_tunnel == &tunnel)
        {
            return;
        }
        if (_tunnel != nullptr)
        {
            throw std::runtime_error("Cannot register tunnel. This window already has one.");
        }
        _tunnel = &tunnel;
    }

    WindowTunnel* Window::getTunnel()
    {
        return _tunnel;
    }

}