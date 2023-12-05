#pragma once


#include <filesystem>
#include <map>

#include <render_engine/containers/BackBuffer.h>
#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/window/Window.h>

namespace RenderEngine
{
    class Material;
    class Mesh;
    class MeshInstance;
    class Technique;
    class Buffer;

    class ForwardRenderer : public SingleColorOutputRenderer
    {
    private:
        struct MeshBuffers
        {
            std::unique_ptr<Buffer> vertex_buffer;
            std::unique_ptr<Buffer> index_buffer;
            std::unique_ptr<Buffer> color_buffer;
            std::unique_ptr<Buffer> normal_buffer;
            std::unique_ptr<Buffer> texture_buffer;
        };
        struct MeshGroup
        {
            std::unique_ptr<Technique> technique;
            std::vector<const MeshInstance*> mesh_instances;
        };
        using MaterialMeshGroupMap = std::unordered_map<uint32_t, MeshGroup>;
    public:
        static constexpr uint32_t kRendererId = 2u;
        ForwardRenderer(Window& parent,
                        const RenderTarget& render_target,
                        bool last_renderer);
        ~ForwardRenderer() override;
        void onFrameBegin(uint32_t frame_number) override;
        void addMesh(const MeshInstance* mesh_instance, int32_t priority);
        void draw(uint32_t swap_chain_image_index, uint32_t frame_number) override;
        std::vector<VkSemaphoreSubmitInfo> getWaitSemaphores(uint32_t frame_number) override
        {
            return {};
        }
    private:
        std::map<int32_t, MaterialMeshGroupMap> _meshes;
        std::map<const Mesh*, MeshBuffers> _mesh_buffers;

    };
}
