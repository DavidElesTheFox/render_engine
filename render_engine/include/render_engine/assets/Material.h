#pragma once

#include <render_engine/assets/Image.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/assets/TextureBindingMap.h>
#include <render_engine/resources/PushConstantsUpdater.h>
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
        struct BlendingInfo
        {
            bool enabled{ false };
            VkBlendFactor src_factor{ VK_BLEND_FACTOR_ONE };
            VkBlendFactor dst_factor{ VK_BLEND_FACTOR_ZERO };
            VkBlendOp op{ VK_BLEND_OP_ADD };

            BlendingInfo clone() const
            {
                return { .enabled = enabled, .src_factor = src_factor, .dst_factor = dst_factor, .op = op };
            }

            BlendingInfo&& setEnabled(bool value)&& { enabled = value; return std::move(*this); }
            BlendingInfo&& setSrcFactor(VkBlendFactor value)&& { src_factor = value; return std::move(*this); }
            BlendingInfo&& setDstFactor(VkBlendFactor value)&& { dst_factor = value; return std::move(*this); }
            BlendingInfo&& setOp(VkBlendOp value)&& { op = value; return std::move(*this); }
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

        const BlendingInfo& getColorBlending() const { return _color_blending; }
        void setColorBlending(BlendingInfo value) { _color_blending = std::move(value); }

        const BlendingInfo& getAlpheBlending() const { return _alpha_blending; }
        void setAlphaBlending(BlendingInfo value) { _alpha_blending = std::move(value); }
    private:

        bool checkPushConstantsConsistency() const;

        std::unique_ptr<Shader> _vertex_shader;
        std::unique_ptr<Shader> _fragment_shader;
        uint32_t _id;
        CallbackContainer _callbacks;
        RasterizationInfo _rasterization_info{};
        BlendingInfo _color_blending{ .enabled = false, .src_factor = VK_BLEND_FACTOR_SRC_ALPHA, .dst_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, .op = VK_BLEND_OP_ADD };
        BlendingInfo _alpha_blending{};
    };

    class MaterialInstance
    {
    public:
        class UpdateContext
        {
        public:
            UpdateContext(const std::vector<UniformBinding*>& per_frame_uniforms,
                          const std::vector<UniformBinding*>& per_draw_call_uniforms,
                          PushConstantsUpdater push_constant_updater,
                          const MaterialInstance& material_instance)
                : _per_frame_uniforms(per_frame_uniforms)
                , _per_draw_call_uniforms(per_draw_call_uniforms)
                , _push_constant_updater(std::move(push_constant_updater))
                , _material_instance(material_instance)
            {}

            const Shader::MetaData& getVertexShaderMetaData() const
            {
                return _material_instance.getMaterial().getVertexShader().getMetaData();
            }
            const Shader::MetaData& getFragmentShaderMetaData() const
            {
                return _material_instance.getMaterial().getFragmentShader().getMetaData();
            }
            const std::vector<UniformBinding*>& getPerFrameUniforms() const { return _per_frame_uniforms; }
            const std::vector<UniformBinding*>& getPerDrawCallUniforms() const { return _per_draw_call_uniforms; }
            PushConstantsUpdater& getPushConstantUpdater() { return _push_constant_updater; }

        private:
            const std::vector<UniformBinding*>& _per_frame_uniforms;
            const std::vector<UniformBinding*>& _per_draw_call_uniforms;
            PushConstantsUpdater _push_constant_updater;
            const MaterialInstance& _material_instance;
        };
        struct CallbackContainer
        {
            std::function<void(UpdateContext& update_context, uint32_t frame_number)> on_frame_begin;
            std::function<void(UpdateContext& update_context, const MeshInstance* mesh_instance)> on_draw;
        };

        MaterialInstance(Material& material,
                         TextureBindingMap texture_bindings,
                         CallbackContainer callbacks,
                         uint32_t id)
            : _material(material)
            , _texture_bindings(std::move(texture_bindings))
            , _callbacks(std::move(callbacks))
            , _id(id)
        {}

        virtual ~MaterialInstance() = default;

        std::unique_ptr<Technique> createTechnique(GpuResourceManager& gpu_resource_manager,
                                                   TextureBindingMap&& subpass_textures,
                                                   VkRenderPass render_pass,
                                                   uint32_t corresponding_subpass) const;

        void onFrameBegin(UpdateContext& update_context, uint32_t frame_number) const
        {
            if (_callbacks.on_frame_begin == nullptr)
            {
                return;
            }
            _callbacks.on_frame_begin(update_context, frame_number);
        }
        void onDraw(UpdateContext& update_context, const MeshInstance* mesh_instance) const
        {
            if (_callbacks.on_draw == nullptr)
            {
                return;
            }
            _callbacks.on_draw(update_context, mesh_instance);
        }
        uint32_t getId() const { return _id; }

        const Material& getMaterial() const { return _material; }
    protected:
        TextureBindingMap cloneBindings() const;
        const CallbackContainer& getCallbackContainer() const { return _callbacks; }
    private:
        Material& _material;
        TextureBindingMap _texture_bindings;
        CallbackContainer _callbacks;
        uint32_t _id{ 0 };
    };
}
