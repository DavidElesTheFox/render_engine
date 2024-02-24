#include <render_engine/window/OffScreenWindow.h>

#include <render_engine/containers/Views.h>
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
        , _back_buffer()
        , _back_buffer_size(textures.size())
        , _image_stream(ImageStream::ImageDescription{ .width = textures.front()->getImage().getWidth(),
                .height = textures.front()->getImage().getHeight(),
                .format = textures.front()->getImage().getFormat() })
    {
        _back_buffer.reserve(_back_buffer_size);
        Texture::ImageViewData image_view_data;

        for (uint32_t i = 0; i < _back_buffer_size; ++i)
        {
            auto& texture = *textures[i];
            _back_buffer.emplace_back(FrameData{
                .synch_render = SyncObject::CreateEmpty(device.getLogicalDevice()),
                .contains_image = false,
                .render_target_texture = std::move(textures[i]),
                .render_target_texture_view = texture.createTextureView(image_view_data, std::nullopt) });
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
                                                                                     createRenderTarget(),
                                                                                     _back_buffer.size());
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
        std::vector<ITextureView*> image_views;
        for (auto& frame_data : _back_buffer)
        {
            image_views.push_back(frame_data.render_target_texture_view.get());
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
        readBack();
        _current_render_target_index = (_current_render_target_index + 1) % _back_buffer_size;
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

        _render_engine->onFrameBegin(renderers, getCurrentImageIndex());

        const std::string operation_group_name = frame_data.contains_image ? SyncGroups::kInternal : SyncGroups::kEmpty;

        bool draw_call_submitted = _render_engine->render(frame_data.synch_render.getOperationsGroup(operation_group_name),
                                                          _renderers | std::views::transform([](const auto& ptr) { return ptr.get(); }),
                                                          getCurrentImageIndex());
        frame_data.contains_image = draw_call_submitted;
    }
    void OffScreenWindow::readBack()
    {
        {
            FrameData& frame_to_download = _back_buffer[getCurrentImageIndex()];
            // Start reading back the current image
            _device.getStagingArea().getScheduler().download(frame_to_download.render_target_texture.get(),
                                                             frame_to_download.synch_render.getOperationsGroup(SyncGroups::kPresent));
        }
        {
            FrameData& frame_to_read_back = _back_buffer[getOldestImageIndex()];

            if (frame_to_read_back.contains_image == false)
            {
                return;
            }

            /*
            * TODO: Current bottleneck: Copying data from a coherent memory is slow. It should be done on a different thread.
            *
            * This TODO should be done when the render graph is introduced and parallel rendering is there.
            */
            auto download_task = frame_to_read_back.render_target_texture->clearDownloadTask();
            assert(download_task != nullptr && "When the frame contains image it should have a download task");

            auto image = download_task->getImage();
            _image_stream << std::move(std::get<std::vector<uint8_t>>(image.getData()));
        }
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