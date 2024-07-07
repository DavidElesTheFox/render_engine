#pragma once

#include <render_engine/assets/Material.h>
#include <render_engine/containers/ImageStream.h>
#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/resources/Technique.h>
#include <render_engine/window/Window.h>

namespace RenderEngine
{
    class ImageStreamRenderer : public SingleColorOutputRenderer
    {
    public:
        static constexpr uint32_t kRendererId = 3u;
        ImageStreamRenderer(IRenderEngine& render_engine,
                            ImageStream& image_stream,
                            RenderTarget render_target,
                            uint32_t back_buffer_size,
                            bool use_internal_command_buffers);
        ~ImageStreamRenderer()
        {
            destroy();
        };

        void onFrameBegin(uint32_t frame_number) final;
        void draw(uint32_t swap_chain_image_index) final;
        void draw(VkCommandBuffer command_buffer, uint32_t swap_chain_image_index) final;

        SyncOperations getSyncOperations(uint32_t image_index) final;
    private:
        struct UploadData
        {
            SyncObject synchronization_object;
            explicit UploadData(SyncObject synchronization_object)
                : synchronization_object(std::move(synchronization_object))
            {}
        };
        void destroy() noexcept;
        bool skipDrawCall(uint32_t frame_number) const override final;
        std::vector<AttachmentInfo> reinitializeAttachments(const RenderTarget&) override final { return {}; }
        ImageStream& _image_stream;
        Image _image_cache;
        std::unordered_map<Texture*, UploadData> _upload_data;
        std::unique_ptr<Material> _fullscreen_material;
        std::unique_ptr<MaterialInstance> _material_instance;
        std::unique_ptr<Technique> _technique;
        std::vector<std::unique_ptr<Texture>> _texture_container;
        bool _draw_call_recorded{ true };
        PerformanceMarkerFactory _performance_markers;

    };
}