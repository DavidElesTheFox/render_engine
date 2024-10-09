#pragma once


#include <filesystem>
#include <map>

#include <render_engine/containers/BackBuffer.h>
#include <render_engine/FrameBuffer.h>
#include <render_engine/memory/RefObj.h>
#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/RenderPass.h>
#include <render_engine/window/Window.h>

namespace RenderEngine
{
    class Material;
    class Mesh;
    class MeshInstance;
    class Technique;
    class Buffer;

    class ForwardRenderer : public AbstractRenderer
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
        ForwardRenderer(RefObj<IRenderEngine> render_engine,
                        RenderTarget render_target,
                        bool use_internal_command_buffers);
        ~ForwardRenderer() override;
        void onFrameBegin(uint32_t image_index) override;
        void addMesh(const MeshInstance* mesh_instance);
        void draw(uint32_t swap_chain_image_index) override;
        void draw(VkCommandBuffer command_buffer, uint32_t swap_chain_image_index) override;
        SyncOperations getSyncOperations(uint32_t) final
        {
            return {};
        }
    private:
        LogicalDevice& getLogicalDevice() { return _render_engine->getDevice().getLogicalDevice(); }
        VkRect2D getRenderArea() const
        {
            return VkRect2D{ .offset {0, 0}, .extent {_render_target.getExtent()} };
        }
        RefObj<IRenderEngine> _render_engine;
        RenderTarget _render_target;
        std::vector<MeshGroup> _meshes;
        std::map<const Mesh*, MeshBuffers> _mesh_buffers;
        PerformanceMarkerFactory _performance_markers;
        std::unique_ptr<RenderPass> _render_pass;
        std::vector<FrameBuffer> _frame_buffers;

        std::vector<VkCommandBuffer> _internal_command_buffers;
    };
}
