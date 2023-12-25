#pragma once

#include <render_engine/assets/Shader.h>
#include <render_engine/assets/TextureBindingMap.h>
#include <render_engine/resources/Texture.h>

#include <variant>

#include <render_engine/containers/VariantOverloaded.h>
namespace RenderEngine
{
    class TextureAssignment
    {
    public:
        struct BindingSlot
        {
            int32_t binding{ -1 };
            std::variant<Shader::MetaData::Sampler, Shader::MetaData::UniformBuffer, Shader::MetaData::InputAttachment> data;
            std::vector<ITextureView*> texture_views;
            VkShaderStageFlags shader_stage{ 0 };
            Shader::MetaData::UpdateFrequency getUpdateFrequency() const
            {
                return std::visit(overloaded{
                    [](const Shader::MetaData::Sampler& sampler) { return sampler.update_frequency; },
                    [](const Shader::MetaData::UniformBuffer& buffer) { return buffer.update_frequency; },
                    [](const Shader::MetaData::InputAttachment&) { return Shader::MetaData::UpdateFrequency::PerFrame; },
                    [](const auto&) { throw std::runtime_error("Frequency is not defined for the binding"); }
                                  },
                                  data);
            }
        };
        explicit TextureAssignment(const std::vector<std::pair<Shader, VkShaderStageFlags>> shaders);

        void assignTextures(const TextureBindingMap& textures, uint32_t back_buffer_size);
        void assignInputAttachments(const TextureBindingMap& textures, uint32_t back_buffer_size);

        std::vector<BindingSlot> getBindings(Shader::MetaData::UpdateFrequency update_frequency) const
        {
            auto result = ((std::vector{ _slots, _subpass_slots }) | std::views::join
                           | std::views::filter([=](const auto& slot) { return slot.getUpdateFrequency() == update_frequency; }));
            return std::vector<BindingSlot>(result.begin(), result.end());
        }

    private:
        void addShader(const Shader& shader, VkShaderStageFlags shader_usage);

        void assignTexture(const std::vector<ITextureView*> texture_views, int32_t sampler_binding);

        void assignTextureToInputAttachment(const std::vector<ITextureView*> texture_views, int32_t sampler_binding);


        std::vector<BindingSlot> _slots;
        std::vector<BindingSlot> _subpass_slots;
    };
}