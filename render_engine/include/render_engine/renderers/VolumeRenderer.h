#pragma once

#include <render_engine/assets/Material.h>
#include <render_engine/assets/VolumetricObject.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/resources/Texture.h>

namespace RenderEngine
{
    class VolumeRenderer : public SingleColorOutputRenderer
    {
    public:
        VolumeRenderer(IWindow& parent,
                       const RenderTarget& render_target,
                       bool last_renderer);
        ~VolumeRenderer() override;
        void onFrameBegin(uint32_t image_index) override;
        void addVolumeObject(const VolumetricObjectInstance* mesh_instance);
        void draw(uint32_t swap_chain_image_index) override;
        std::vector<VkSemaphoreSubmitInfo> getWaitSemaphores(uint32_t image_index) override
        {
            return {};
        }
    private:
        struct FrameBufferData
        {
            std::vector<std::unique_ptr<Texture>> back_buffer_textures;
            std::vector<std::unique_ptr<TextureView>> back_buffer_views;
            std::unique_ptr<RenderTarget> render_target;

        };
        struct RenderPassData
        {
            std::unique_ptr<ForwardRenderer> renderer;
            FrameBufferData frame_buffer;
            std::unique_ptr<Material> material;
            std::unique_ptr<MaterialInstance> material_instance;
            std::unique_ptr<MeshInstance> cube_mesh_instance;
            std::unique_ptr<Mesh> cube_mesh;
        };
        struct DrawData
        {
            RenderPassData _front_face_pass;
            RenderPassData _back_face_pass;
            const VolumetricObjectInstance* mesh_instances{ nullptr };
        };

        static RenderPassData createPrePass(bool render_front_face);
        static FrameBufferData createFrameBufferData(uint32_t back_buffer_count);

        std::vector<DrawData> _meshes;
        std::unique_ptr<ForwardRenderer> _volume_renderer;
    };
}