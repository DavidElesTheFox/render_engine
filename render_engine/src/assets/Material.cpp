#include <render_engine/assets/Material.h>

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/Device.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/GpuResourceSet.h>
#include <render_engine/resources/Technique.h>

#include "render_engine/assets/TextureAssignment.h"

#include <functional>
#include <numeric>
#include <ranges>
#include <set>
#include <variant>

namespace RenderEngine
{
    namespace
    {
        struct UniformCreationData
        {
            BackBuffer<UniformBinding::FrameData> back_buffer;
            int32_t binding{ -1 };
        };

    }


    Material::Material(std::unique_ptr<Shader> verted_shader,
                       std::unique_ptr<Shader> fragment_shader,
                       CallbackContainer callbacks,
                       uint32_t id)
        : _vertex_shader(std::move(verted_shader))
        , _fragment_shader(std::move(fragment_shader))
        , _id{ id }
        , _callbacks(std::move(callbacks))
    {
        if (checkPushConstantsConsistency() == false)
        {
            //	throw std::runtime_error("Materials push constants are not consistant across the shaders.");
        }
    }

    bool Material::checkPushConstantsConsistency() const
    {
        if (_vertex_shader->getMetaData().push_constants.has_value()
            && _fragment_shader->getMetaData().push_constants.has_value())
        {
            return _vertex_shader->getMetaData().push_constants->size == _fragment_shader->getMetaData().push_constants->size;
        }
        return true;
    }

    std::unique_ptr<Technique> MaterialInstance::createTechnique(GpuResourceManager& gpu_resource_manager,
                                                                 TextureBindingMap&& subpass_textures,
                                                                 VkRenderPass render_pass,
                                                                 uint32_t corresponding_subpass) const
    {
        const uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();
        const Shader& vertex_shader = _material.getVertexShader();
        const Shader& fragment_shader = _material.getFragmentShader();

        TextureAssignment texture_assignment({
            { vertex_shader, VK_SHADER_STAGE_VERTEX_BIT },
            { fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT} });
        texture_assignment.assignTextures(_texture_bindings, back_buffer_size);
        texture_assignment.assignInputAttachments(subpass_textures, back_buffer_size);
        GpuResourceSet constant_resources(gpu_resource_manager,
                                          texture_assignment.getBindings(Shader::MetaData::UpdateFrequency::Constant));
        GpuResourceSet per_frame_resources(gpu_resource_manager,
                                           texture_assignment.getBindings(Shader::MetaData::UpdateFrequency::PerFrame));
        GpuResourceSet per_draw_call_resources(gpu_resource_manager,
                                               texture_assignment.getBindings(Shader::MetaData::UpdateFrequency::PerDrawCall));
        return std::make_unique<Technique>(gpu_resource_manager.getLogicalDevice(),
                                           this,
                                           std::move(subpass_textures),
                                           std::move(constant_resources),
                                           std::move(per_frame_resources),
                                           std::move(per_draw_call_resources),
                                           render_pass,
                                           corresponding_subpass);
    }

    TextureBindingMap MaterialInstance::cloneBindings() const
    {
        return _texture_bindings.clone();
    }
}
