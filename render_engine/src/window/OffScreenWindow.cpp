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
    namespace SyncGroups
    {
        constexpr auto kEmpty = "EmptyGroup";
        constexpr auto kPresent = "PresentGroup";
    }

    OffScreenWindow::OffScreenWindow(Device& device,
                                     std::unique_ptr<RenderEngine>&& render_engine,
                                     std::vector<std::unique_ptr<Texture>>&& textures)
        try : _device(device)
        , _render_engine(std::move(render_engine))
        , _textures(std::move(textures))
        , _back_buffer()
        , _back_buffer_size(_textures.size())
        , _image_stream(ImageStream::ImageDescription{ .width = _textures.front()->getImage().getWidth(),
                .height = _textures.front()->getImage().getHeight(),
                .format = _textures.front()->getImage().getFormat() })
    {
        _back_buffer.reserve(_back_buffer_size);
        for (uint32_t i = 0; i < _back_buffer_size; ++i)
        {
            _back_buffer.emplace_back(FrameData{
                .synch_render = SyncObject::CreateWithFence(device.getLogicalDevice(), 0) });
        }
        Texture::ImageViewData image_view_data;
        for (auto& texture : _textures)
        {
            _texture_views.emplace_back(std::make_unique<TextureView>(*texture,
                                                                      image_view_data,
                                                                      std::nullopt,
                                                                      getDevice().getPhysicalDevice(),
                                                                      getDevice().getLogicalDevice()));
        }
        initSynchronizationObjects();
    }
    catch (const std::runtime_error&)
    {
        destroy();
    }

    OffScreenWindow::~OffScreenWindow()
    {
        destroy();
    }

    void OffScreenWindow::update()
    {
        RenderContext::context().clearGarbage();
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
            frame_data.synch_render.createSemaphore("image-available");
            frame_data.synch_render.createSemaphore("render-finished");

            frame_data.synch_render.addSignalOperationToGroup(SyncGroups::kPresent,
                                                              "image-available",
                                                              VK_PIPELINE_STAGE_2_COPY_BIT);
            frame_data.synch_render.addWaitOperationToGroup(SyncGroups::kInternal,
                                                            "image-available",
                                                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

            frame_data.synch_render.addSignalOperationToGroup(SyncGroups::kInternal,
                                                              "render-finished",
                                                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            frame_data.synch_render.addSignalOperationToGroup(SyncGroups::kEmpty,
                                                              "render-finished",
                                                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            frame_data.synch_render.addWaitOperationToGroup(SyncGroups::kPresent,
                                                            "render-finished",
                                                            VK_PIPELINE_STAGE_2_COPY_BIT);


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
        auto& logical_device = _device.getLogicalDevice();
        logical_device->vkDeviceWaitIdle(*logical_device);
        _back_buffer.clear();
        _renderers.clear();

        _render_engine.reset();
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

    void OffScreenWindow::present(FrameData& frame_data) const
    {
        if (_renderers.empty())
        {
            return;
        }
        auto renderers = _renderers | std::views::transform([](const auto& ptr) { return ptr.get(); });

        /*
        * TODO we need to ensure that the previous draw call is finished.Currently download is a blocking command, thus it is an implicit guarantee.
        * Probably we should wait here the fence what currently waited inside the download.
        */
        assert(frame_data.synch_render.getOperationsGroup(SyncGroups::kInternal).hasAnyFence() && "Render sync operation needs a fence");
        /*vkWaitForFences(logical_device, 1, frame_data.synch_render.getOperationsGroup(SyncGroups::kInternal).getFence(), VK_TRUE, UINT64_MAX);

        vkResetFences(logical_device, 1, frame_data.synch_render.getOperationsGroup(SyncGroups::kInternal).getFence());
        */
        _render_engine->onFrameBegin(renderers, getCurrentImageIndex());

        const std::string operation_group_name = frame_data.contains_image ? SyncGroups::kInternal : SyncGroups::kEmpty;

        bool draw_call_submitted = _render_engine->render(frame_data.synch_render.getOperationsGroup(operation_group_name).extract(~SyncOperations::ExtractFence),
                                                          _renderers | std::views::transform([](const auto& ptr) { return ptr.get(); }),
                                                          getCurrentImageIndex());
        frame_data.contains_image = draw_call_submitted;
    }
    void OffScreenWindow::readBack(FrameData& frame)
    {
        // Start reading back the current image
        _device.getStagingArea().getScheduler().download(_textures[getCurrentImageIndex()].get(),
                                                         _render_engine->getCommandContext());
        if (frame.contains_image == false)
        {
            return;
        }

        auto download_task = _textures[getOldestImageIndex()]->clearDownloadTask();
        assert(download_task != nullptr && "When the frame contains image it should have a download task");

        auto image = download_task->getImage();
        _image_stream << std::move(std::get<std::vector<uint8_t>>(image.getData()));
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

    TextureFactory& OffScreenWindow::getTextureFactory()
    {
        return _device.getTextureFactory();
    }
}