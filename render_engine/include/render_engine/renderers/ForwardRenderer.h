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
    public:
        static constexpr uint32_t kRendererId = 2u;
        ForwardRenderer(IRenderEngine& render_engine,
                        RenderTarget render_target);
        ~ForwardRenderer() override;
        void onFrameBegin(uint32_t image_index) override;
        void addMesh(const MeshInstance* mesh_instance);
        void draw(uint32_t swap_chain_image_index) override;
        SyncOperations getSyncOperations(uint32_t) final
        {
            return {};
        }
    private:
        std::vector<AttachmentInfo> reinitializeAttachments(const RenderTarget&) override final { return {}; }

        std::vector<MeshGroup> _meshes;
        std::map<const Mesh*, MeshBuffers> _mesh_buffers;
        PerformanceMarkerFactory _performance_markers;

    };
}
