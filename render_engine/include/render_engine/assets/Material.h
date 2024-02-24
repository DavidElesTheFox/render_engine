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
                 uint32_t id,
                 std::string name);
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
        const std::string& getName() const { return _name; }
    private:


        std::unique_ptr<Shader> _vertex_shader;
        std::unique_ptr<Shader> _fragment_shader;
        uint32_t _id;
        CallbackContainer _callbacks;
        std::string _name;
        RasterizationInfo _rasterization_info{};
        BlendingInfo _color_blending{ .enabled = false, .src_factor = VK_BLEND_FACTOR_SRC_ALPHA, .dst_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, .op = VK_BLEND_OP_ADD };
        BlendingInfo _alpha_blending{};
    };


}
