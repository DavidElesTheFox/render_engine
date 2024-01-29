#pragma once

#include <render_engine/assets/Material.h>
#include <render_engine/assets/VolumetricObject.h>
#include <render_engine/cuda_compute/DistanceFieldTask.h>
#include <render_engine/cuda_compute/ExternalSurface.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/SingleColorOutputRenderer.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/resources/Texture.h>

namespace RenderEngine
{
    class VolumeRenderer : public SingleColorOutputRenderer
    {
    public:
        static constexpr uint32_t kRendererId = 4u;

        VolumeRenderer(IWindow& window,
                       const RenderTarget& render_target,
                       bool last_renderer);
        ~VolumeRenderer() override = default;
        void onFrameBegin(uint32_t image_index) override;
        void addVolumeObject(const VolumetricObjectInstance* mesh_instance);
        void draw(uint32_t swap_chain_image_index) override;
        SyncOperations getSyncOperations(uint32_t image_index) final;
    private:
        struct MeshBuffers
        {
            std::unique_ptr<Buffer> vertex_buffer;
            std::unique_ptr<Buffer> index_buffer;
            std::unique_ptr<Buffer> color_buffer;
            std::unique_ptr<Buffer> normal_buffer;
            std::unique_ptr<Buffer> texture_buffer;
        };
        struct FrameBufferData
        {
            std::vector<std::unique_ptr<Texture>> textures_per_back_buffer;
            std::vector<std::unique_ptr<TextureView>> texture_views_per_back_buffer;
        };
        struct TechniqueData
        {
            std::vector<std::unique_ptr<Material>> subpass_materials;
            std::vector<std::unique_ptr<MaterialInstance>> subpass_material_instance;
            std::unique_ptr<Technique> front_face_technique;
            std::unique_ptr<Technique> back_face_technique;
            std::unique_ptr<Technique> volume_technique;
            std::vector<std::unique_ptr<CudaCompute::ExternalSurface>> distance_field_surface;
            std::vector<std::unique_ptr<CudaCompute::ExternalSurface>> intensity_surface;
            std::vector<std::unique_ptr<Texture>> distance_field_textures;
            std::vector<std::unique_ptr<TextureView>> distance_field_texture_views;
            std::vector<SynchronizationObject> synchronization_objects;
            uint32_t segmentation_threshold{};
        };
        struct MeshGroup
        {
            TechniqueData technique_data;
            std::vector<const VolumetricObjectInstance*> meshes;
            std::vector<CudaCompute::DistanceFieldTask> tasks;
        };

        void initializeFrameBuffers(uint32_t back_buffer_count, const Image& ethalon_image);
        void initializeFrameBufferData(uint32_t back_buffer_count, const Image& ethalon_image, FrameBufferData* frame_buffer_data);
        TechniqueData createTechniqueDataFor(const VolumetricObjectInstance& mesh);
        std::vector<AttachmentInfo> reinitializeAttachments(const RenderTarget& render_target) override final;
        std::vector<AttachmentInfo> createFrameBuffersAndAttachments(const RenderTarget& render_target);
        void resetResourceStatesOf(Technique& technique, uint32_t image_index);
        void drawWithTechnique(const std::string& subpass_name,
                               Technique& technique,
                               const std::vector<const VolumetricObjectInstance*>& meshes,
                               FrameData& frame_data,
                               uint32_t swap_chain_image_index);
        void renderMeshGroup(MeshGroup& mesh_group, uint32_t swap_chain_image_index, FrameData& frame_data, bool calculate_distance_field);
        void startDistanceFieldTask(MeshGroup& mesh_group, uint32_t swap_chain_image_index);
        void cleanupDistanceFieldTasks(MeshGroup& mesh_group);

        RenderTarget _render_target;
        std::vector<MeshGroup> _meshes;
        std::vector<MeshGroup> _meshes_with_distance_field;
        FrameBufferData _front_face_frame_buffer;
        FrameBufferData _back_face_frame_buffer;
        std::map<const Mesh*, MeshBuffers> _mesh_buffers;
        PerformanceMarkerFactory _performance_markers;
        TextureFactory _texture_factory;

    };
}