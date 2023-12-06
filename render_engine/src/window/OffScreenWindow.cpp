#include <render_engine/window/OffScreenWindow.h>

#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

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
    OffScreenWindow::OffScreenWindow(Device& device,
                                     std::unique_ptr<RenderEngine>&& render_engine,
                                     std::unique_ptr<TransferEngine>&& transfer_engine,
                                     std::vector<std::unique_ptr<Texture>>&& textures)
        : _device(device)
        , _render_engine(std::move(render_engine))
        , _transfer_engine(std::move(transfer_engine))
        , _textures(std::move(textures))
        , _back_buffer(_textures.size(), FrameData{})
        , _back_buffer_size(_textures.size())
        , _image_stream(ImageStream::ImageDescription{ .width = _textures.front()->getImage().getWidth(),
                .height = _textures.front()->getImage().getHeight(),
                .format = _textures.front()->getImage().getFormat() })
    {
        Texture::ImageViewData image_view_data;
        for (auto& texture : _textures)
        {
            _texture_views.emplace_back(std::make_unique<TextureView>(*texture,
                                                                      texture->createImageView(image_view_data),
                                                                      VK_NULL_HANDLE,
                                                                      getDevice().getPhysicalDevice(),
                                                                      getDevice().getLogicalDevice()));
        }
        initSynchronizationObjects();
    }

    OffScreenWindow::~OffScreenWindow()
    {
        destroy();
    }

    void OffScreenWindow::update()
    {
        present();
    }

    void OffScreenWindow::registerRenderers(const std::vector<uint32_t>& renderer_ids)
    {
        _renderers = RenderContext::context().getRendererFactory().generateRenderers(renderer_ids,
                                                                                     *this,
                                                                                     createRenderTarget(), _back_buffer.size());
        for (size_t i = 0; i < renderer_ids.size(); ++i)
        {
            _renderer_map[renderer_ids[i]] = _renderers[i].get();
        }
    }

    void OffScreenWindow::initSynchronizationObjects()
    {
        for (FrameData& frame_data : _back_buffer)
        {
            VkSemaphoreCreateInfo semaphore_info{};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            auto logical_device = _device.getLogicalDevice();
            if (vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &frame_data.image_available_semaphore) != VK_SUCCESS ||
                vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &frame_data.render_finished_semaphore) != VK_SUCCESS ||
                vkCreateFence(logical_device, &fence_info, nullptr, &frame_data.in_flight_fence) != VK_SUCCESS)
            {
                vkDestroySemaphore(logical_device, frame_data.image_available_semaphore, nullptr);
                vkDestroySemaphore(logical_device, frame_data.render_finished_semaphore, nullptr);
                vkDestroyFence(logical_device, frame_data.in_flight_fence, nullptr);
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void OffScreenWindow::enableRenderdocCapture()
    {
        if (_renderdoc_api != nullptr)
        {
            return;
        }
        _renderdoc_api = RenderContext::context().getRenderdocApi();
    }

    void OffScreenWindow::disableRenderdocCapture()
    {
        _renderdoc_api = nullptr;
    }
    void OffScreenWindow::destroy()
    {
        auto logical_device = _device.getLogicalDevice();
        vkDeviceWaitIdle(logical_device);
        for (FrameData& frame_data : _back_buffer)
        {
            vkDestroySemaphore(logical_device, frame_data.image_available_semaphore, nullptr);
            vkDestroySemaphore(logical_device, frame_data.render_finished_semaphore, nullptr);
            vkDestroyFence(logical_device, frame_data.in_flight_fence, nullptr);
        }
        _renderers.clear();

        _render_engine.reset();
        _transfer_engine.reset();
    }

    RenderTarget OffScreenWindow::createRenderTarget()
    {
        std::vector<VkImageView> image_views;
        for (auto& image_view : _texture_views)
        {
            image_views.push_back(image_view->getImageView());
        }
        const auto& image_stream_description = _image_stream.getImageDescription();
        return { std::move(image_views),
            image_stream_description.width,
            image_stream_description.height,
            image_stream_description.format,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        };

    }

    void OffScreenWindow::present()
    {
        if (_renderdoc_api) ((RENDERDOC_API_1_1_2*)_renderdoc_api)->StartFrameCapture(NULL, NULL);

        present(_back_buffer[getCurrentImageIndex()]);
        readBack(_back_buffer[getOldestImageIndex()]);
        _current_render_target_index = (_current_render_target_index + 1) % _textures.size();
        if (_renderdoc_api) ((RENDERDOC_API_1_1_2*)_renderdoc_api)->EndFrameCapture(NULL, NULL);

        _frame_counter++;
    }

    void OffScreenWindow::present(FrameData& frame_data)
    {
        if (_renderers.empty())
        {
            return;
        }
        auto renderers = _renderers | std::views::transform([](const auto& ptr) { return ptr.get(); });

        auto logical_device = _device.getLogicalDevice();
        vkWaitForFences(logical_device, 1, &frame_data.in_flight_fence, VK_TRUE, UINT64_MAX);

        vkResetFences(logical_device, 1, &frame_data.in_flight_fence);
        _render_engine->onFrameBegin(renderers, getCurrentImageIndex());

        SynchronizationPrimitives synchronization_primitives{};
        if (frame_data.contains_image)
        {
            VkSemaphoreSubmitInfo wait_semaphore_info{};
            wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            wait_semaphore_info.semaphore = frame_data.image_available_semaphore;
            wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            synchronization_primitives.wait_semaphores.push_back(wait_semaphore_info);
        }
        {
            VkSemaphoreSubmitInfo signal_semaphore_info{};
            signal_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signal_semaphore_info.semaphore = frame_data.render_finished_semaphore;
            signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

            synchronization_primitives.signal_semaphores.push_back(signal_semaphore_info);
        }
        synchronization_primitives.on_finished_fence = frame_data.in_flight_fence;

        bool draw_call_submitted = _render_engine->render(&synchronization_primitives,
                                                          _renderers | std::views::transform([](const auto& ptr) { return ptr.get(); }),
                                                          getCurrentImageIndex());
        frame_data.contains_image = draw_call_submitted;
    }
    void OffScreenWindow::readBack(FrameData& frame)
    {
        if (frame.contains_image == false)
        {
            return;
        }
        SynchronizationPrimitives synchronization_primitives = SynchronizationPrimitives::CreateWithFence(_device.getLogicalDevice());
        {
            VkSemaphoreSubmitInfo semaphore_submit_info{};
            semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            semaphore_submit_info.semaphore = frame.render_finished_semaphore;
            semaphore_submit_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            synchronization_primitives.wait_semaphores.push_back(semaphore_submit_info);
        }
        {
            VkSemaphoreSubmitInfo semaphore_submit_info{};
            semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            semaphore_submit_info.semaphore = frame.image_available_semaphore;
            semaphore_submit_info.stageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            synchronization_primitives.signal_semaphores.push_back(semaphore_submit_info);
        }
        std::vector<uint8_t> image_data = _textures[getOldestImageIndex()]->download(synchronization_primitives,
                                                                                     *_transfer_engine);

        vkDestroyFence(_device.getLogicalDevice(), synchronization_primitives.on_finished_fence, nullptr);
        _image_stream << std::move(image_data);
    }

    void OffScreenWindow::registerTunnel(WindowTunnel& tunnel)
    {
        if (_tunnel != nullptr)
        {
            throw std::runtime_error("Cannot register tunnel. This window already has one.");
        }
        _tunnel = &tunnel;
    }

    WindowTunnel* OffScreenWindow::getTunnel()
    {
        return _tunnel;
    }
}