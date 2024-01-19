#include "render_engine/assets/TextureAssignment.h"

#include <render_engine/containers/VariantOverloaded.h>

namespace RenderEngine
{
    TextureAssignment::TextureAssignment(const std::vector<std::pair<Shader, VkShaderStageFlags>> shaders)
    {
        for (const auto& [shader, shader_usage] : shaders)
        {
            addShader(shader, shader_usage);
        }
    }

    void TextureAssignment::assignTextures(const TextureBindingMap& textures, uint32_t back_buffer_size)
    {
        auto texture_views_per_binding = textures.collectTextureViews(back_buffer_size);
        for (auto& [binding, texture_views] : texture_views_per_binding)
        {
            assignTexture(texture_views, binding);
        }
    }

    void TextureAssignment::assignInputAttachments(const TextureBindingMap& textures, uint32_t back_buffer_size)
    {
        auto texture_views_per_binding = textures.collectTextureViews(back_buffer_size);
        for (auto& [binding, texture_views] : texture_views_per_binding)
        {
            assignTextureToInputAttachment(texture_views, binding);
        }
    }

    void TextureAssignment::addShader(const Shader& shader, VkShaderStageFlags shader_usage)
    {
        for (const auto& [binding, uniform_buffer] : shader.getMetaData().global_uniform_buffers)
        {
            auto it = std::ranges::find_if(_slots, [&](const auto& slot) { return slot.binding == binding; });
            if (it != _slots.end())
            {
                std::visit(overloaded{
                    [](const Shader::MetaData::Sampler& sampler) { throw std::runtime_error("Uniform buffer is already bounded as image sampler"); },
                    [](const Shader::MetaData::InputAttachment&) { throw std::runtime_error("Uniform buffer is already bounded as image sampler"); },
                    [&](const Shader::MetaData::UniformBuffer& buffer) { if (buffer.size != uniform_buffer.size) { throw std::runtime_error("Uniform buffer perviously used a different size"); } }
                           }, it->data);
                it->shader_stage |= shader_usage;
            }
            BindingSlot data{
                .binding = binding, .data = { uniform_buffer }, .shader_stage = shader_usage
            };
            assert(data.getUpdateFrequency() != Shader::MetaData::UpdateFrequency::Unknown);
            _slots.emplace_back(std::move(data));
        }
        for (const auto& [binding, sampler] : shader.getMetaData().samplers)
        {
            auto it = std::ranges::find_if(_slots, [&](const auto& slot) { return slot.binding == binding; });
            if (it != _slots.end())
            {
                std::visit(overloaded{
                    [](const Shader::MetaData::Sampler&) {},
                    [](const Shader::MetaData::UniformBuffer&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); },
                    [](const Shader::MetaData::InputAttachment&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); }
                           }, it->data);
                it->shader_stage |= shader_usage;
            }
            BindingSlot data{
                .binding = binding, .data = { sampler }, .shader_stage = shader_usage
            };
            assert(data.getUpdateFrequency() != Shader::MetaData::UpdateFrequency::Unknown);

            _slots.emplace_back(std::move(data));
        }
        for (const auto& [binding, input_attachment] : shader.getMetaData().input_attachments)
        {
            auto it = std::ranges::find_if(_subpass_slots, [&](const auto& slot) { return slot.binding == binding; });
            if (it != _subpass_slots.end())
            {
                std::visit(overloaded{
                    [](const Shader::MetaData::InputAttachment&) {},
                    [](const Shader::MetaData::UniformBuffer&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); },
                    [](const Shader::MetaData::Sampler&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); }
                           }, it->data);
                it->shader_stage |= shader_usage;
            }
            BindingSlot data{
                .binding = binding, .data = { input_attachment }, .shader_stage = shader_usage
            };
            assert(data.getUpdateFrequency() != Shader::MetaData::UpdateFrequency::Unknown);

            _subpass_slots.emplace_back(std::move(data));
        }
    }
    void TextureAssignment::assignTexture(const std::vector<ITextureView*> texture_views, int32_t sampler_binding)
    {
        auto it = std::ranges::find_if(_slots, [&](const auto& slot) { return slot.binding == sampler_binding; });
        if (it == _slots.end())
        {
            throw std::runtime_error("There is no image sampler for the given binding");
        }
        if (std::holds_alternative<Shader::MetaData::Sampler>(it->data) == false
            && std::holds_alternative<Shader::MetaData::InputAttachment>(it->data) == false)
        {
            throw std::runtime_error("Binding is not an image sampler, couldn't bind image for it");
        }
        it->texture_views = texture_views;
    }
    void TextureAssignment::assignTextureToInputAttachment(const std::vector<ITextureView*> texture_views, int32_t sampler_binding)
    {
        auto it = std::ranges::find_if(_subpass_slots, [&](const auto& slot) { return slot.binding == sampler_binding; });
        if (it == _subpass_slots.end())
        {
            throw std::runtime_error("There is no input attachment for the given binding");
        }
        if (std::holds_alternative<Shader::MetaData::Sampler>(it->data) == false
            && std::holds_alternative<Shader::MetaData::InputAttachment>(it->data) == false)
        {
            throw std::runtime_error("Binding is not an image sampler, couldn't bind image for it");
        }
        it->texture_views = texture_views;
    }
}