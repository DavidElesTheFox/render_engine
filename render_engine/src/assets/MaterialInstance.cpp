#include <render_engine/assets/MaterialInstance.h>

#include <render_engine/assets/TextureAssignment.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/GpuResourceSet.h>
#include <render_engine/resources/Technique.h>

namespace RenderEngine
{
    std::unique_ptr<Technique> MaterialInstance::createTechnique(GpuResourceManager& gpu_resource_manager,
                                                                 TextureBindingMap&& subpass_textures,
                                                                 VkRenderPass render_pass,
                                                                 uint32_t corresponding_subpass,
                                                                 TextureBindingMap&& additional_bindings) const
    {
        const uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();
        const Shader& vertex_shader = _material.getVertexShader();
        const Shader& fragment_shader = _material.getFragmentShader();

        TextureAssignment texture_assignment({
            { vertex_shader, VK_SHADER_STAGE_VERTEX_BIT },
            { fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT} });
        if (additional_bindings.isEmpty() == false)
        {
            texture_assignment.assignTextures(additional_bindings.merge(_texture_bindings.clone()), back_buffer_size);
        }
        else
        {
            texture_assignment.assignTextures(_texture_bindings, back_buffer_size);
        }
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