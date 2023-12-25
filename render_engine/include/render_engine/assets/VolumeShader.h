#pragma once

#include <render_engine/assets/Shader.h>

#include <glm/vec3.hpp>

namespace RenderEngine
{
    class VolumeShader : public Shader
    {
    public:
        class MetaDataExtension
        {
        public:
            using MetaData = Shader::MetaData;
            using UniformBuffer = MetaData::UniformBuffer;
            using Sampler = MetaData::Sampler;
            using PushConstants = MetaData::PushConstants;
            using InputAttachment = MetaData::InputAttachment;

            static constexpr uint32_t kFrontFaceTextureBinding = 0;
            static constexpr uint32_t kBackFaceTextureBinding = 1;

            static MetaDataExtension createForVertexShader();

            static MetaDataExtension createForFragmentShader();

            void addUniformBuffer(uint32_t binding, UniformBuffer buffer)
            {
                _meta_data.global_uniform_buffers[binding] = std::move(buffer);
            }

            void addSampler(uint32_t binding, Sampler sampler);

            void setPushConstants(PushConstants push_constants)
            {
                _meta_data.push_constants = push_constants;
            }

            const MetaData& getMetaData() const { return _meta_data; }
        private:
            MetaDataExtension() = default;
            MetaData _meta_data;
        };
        VolumeShader(const std::filesystem::path& spirv_path, const MetaDataExtension& meta_data_extension)
            :Shader(spirv_path, meta_data_extension.getMetaData())
        {

        }
        VolumeShader(std::span<const uint32_t> spirv_code, const MetaDataExtension& meta_data_extension)
            :Shader(spirv_code, meta_data_extension.getMetaData())
        {

        }
        ~VolumeShader() override = default;
    };
}