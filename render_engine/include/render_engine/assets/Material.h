#pragma once

#include <render_engine/assets/Image.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/resources/Texture.h>


#include <volk.h>

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace RenderEngine
{
    class GpuResourceManager;
    class Technique;
    class UniformBinding;
    class PushConstantsUpdater;
    class Mesh;
    class MeshInstance;
    struct Geometry;

    class Material
    {
    public:
        struct CallbackContainer
        {
            std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> create_vertex_buffer;
        };
        struct RasterizationInfo
        {
            VkFrontFace front_face{ VK_FRONT_FACE_CLOCKWISE };
            VkCullModeFlags cull_mode{ VK_CULL_MODE_BACK_BIT };

            RasterizationInfo clone() const
            {
                return { .front_face = front_face, .cull_mode = cull_mode };
            }

            RasterizationInfo&& setFrontFace(VkFrontFace value)&& { front_face = value; return std::move(*this); }
            RasterizationInfo&& setCullMode(VkCullModeFlags value)&& { cull_mode = value; return std::move(*this); }
        };
        Material(std::unique_ptr<Shader> verted_shader,
                 std::unique_ptr<Shader> fragment_shader,
                 CallbackContainer callbacks,
                 uint32_t id);
        virtual ~Material() = default;
        const Shader& getVertexShader() const { return *_vertex_shader; }
        const Shader& getFragmentShader() const { return *_fragment_shader; }

        uint32_t getId() const { return _id; }

        std::vector<uint8_t> createVertexBufferFromGeometry(const Geometry& geometry) const
        {
            return _callbacks.create_vertex_buffer(geometry, *this);
        }

        std::unordered_map<VkShaderStageFlags, Shader::MetaData::PushConstants> getPushConstantsMetaData() const
        {
            std::unordered_map<VkShaderStageFlags, Shader::MetaData::PushConstants> result;
            if (const auto& push_constants = _vertex_shader->getMetaData().push_constants; push_constants != std::nullopt)
            {
                result.insert({ VK_SHADER_STAGE_VERTEX_BIT | push_constants->shared_with, *push_constants });
            }
            if (const auto& push_constants = _fragment_shader->getMetaData().push_constants; push_constants != std::nullopt)
            {
                result.insert({ VK_SHADER_STAGE_FRAGMENT_BIT | push_constants->shared_with, *push_constants });
            }
            return result;
        }
        const RasterizationInfo& getRasterizationInfo() const { return _rasterization_info; }
        void setRasterizationInfo(RasterizationInfo value) { _rasterization_info = std::move(value); }
    private:

        bool checkPushConstantsConsistency() const;

        std::unique_ptr<Shader> _vertex_shader;
        std::unique_ptr<Shader> _fragment_shader;
        uint32_t _id;
        CallbackContainer _callbacks;
        RasterizationInfo _rasterization_info{};
    };

    class MaterialInstance
    {
    public:
        struct CallbackContainer
        {
            std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> global_ubo_update;
            std::function<void(PushConstantsUpdater& updater)> global_push_constants_update;
            std::function<void(const MeshInstance* mesh_instance, PushConstantsUpdater& updater)> push_constants_updater;
        };

        class TextureBindingData
        {
        public:
            TextureBindingData() = default;
            explicit TextureBindingData(std::unordered_map<int32_t, std::unique_ptr<ITextureView>> general_bindings)
                : _general_texture_bindings(std::move(general_bindings))
            {}
            explicit TextureBindingData(std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> back_buffered_bindings)
                : _back_buffered_texture_bindings(std::move(back_buffered_bindings))
            {}

            std::unordered_map<int32_t, std::vector<ITextureView*>> createTextureViews(int32_t back_buffer_size) const;

            TextureBindingData clone() const;
        private:
            std::unordered_map<int32_t, std::unique_ptr<ITextureView>> _general_texture_bindings;
            std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> _back_buffered_texture_bindings;
        };

        MaterialInstance(Material& material,
                         TextureBindingData texture_bindings,
                         CallbackContainer callbacks,
                         uint32_t id)
            : _material(material)
            , _texture_bindings(std::move(texture_bindings))
            , _callbacks(std::move(callbacks))
            , _id(id)
        {}

        virtual ~MaterialInstance() = default;

        std::unique_ptr<Technique> createTechnique(GpuResourceManager& gpu_resource_manager,
                                                   TextureBindingData&& subpass_textures,
                                                   VkRenderPass render_pass,
                                                   uint32_t corresponding_subpass) const;

        void updateGlobalUniformBuffer(std::vector<UniformBinding>& ubo_container, uint32_t frame_number) const
        {
            _callbacks.global_ubo_update(ubo_container, frame_number);
        }

        void updateGlobalPushConstants(PushConstantsUpdater& updater) const
        {
            _callbacks.global_push_constants_update(updater);
        }
        void updatePushConstants(const MeshInstance* mesh_instance, PushConstantsUpdater& updater) const
        {
            _callbacks.push_constants_updater(mesh_instance, updater);
        }
        uint32_t getId() const { return _id; }

        const Material& getMaterial() const { return _material; }
    protected:
        TextureBindingData cloneBindings() const;
        const CallbackContainer& getCallbackContainer() const { return _callbacks; }
    private:
        Material& _material;
        TextureBindingData _texture_bindings;
        CallbackContainer _callbacks;
        uint32_t _id{ 0 };
    };
}
