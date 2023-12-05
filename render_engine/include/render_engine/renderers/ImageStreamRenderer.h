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
        ImageStreamRenderer(Window& window,
                            ImageStream& image_stream,
                            const RenderTarget& render_target,
                            uint32_t back_buffer_size,
                            bool last_renderer);
        ~ImageStreamRenderer()
        {
            destroy();
        };
        std::vector<VkSemaphoreSubmitInfo> getWaitSemaphores(uint32_t frame_number) override;
    private:
        struct UploadData
        {
            std::optional<TransferEngine::InFlightData> inflight_data;
            SynchronizationPrimitives synchronization_primitives;
            UploadData(TransferEngine::InFlightData inflight_data,
                       SynchronizationPrimitives synchronization_primitives)
                : inflight_data(std::move(inflight_data))
                , synchronization_primitives(synchronization_primitives)
            {}
        };
        void destroy() noexcept;
        void onFrameBegin(uint32_t frame_number) override;
        void draw(uint32_t swap_chain_image_index) override;
        bool skipDrawCall(uint32_t frame_number) const override;

        ImageStream& _image_stream;
        Image _image_cache;
        std::unordered_map<Texture*, UploadData> _upload_data;
        std::unique_ptr<Material> _fullscreen_material;
        std::unordered_map<Texture*, std::unique_ptr<MaterialInstance>> _material_instances;
        std::unordered_map<Texture*, std::unique_ptr<Technique>> _techniques;
        std::vector<std::unique_ptr<Texture>> _texture_container;
        bool _draw_call_recorded{ true };
    };
}