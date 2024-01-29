#include <render_engine/assets/VolumeShader.h>

namespace RenderEngine
{
    VolumeShader::MetaDataExtension VolumeShader::MetaDataExtension::createForVertexShader()
    {
        MetaDataExtension extension;
        extension._meta_data.attributes_stride = sizeof(glm::vec3) // position
            + sizeof(glm::vec3); // 3d texture coordinates
        extension._meta_data.input_attributes.push_back(MetaData::Attribute{ .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 });
        extension._meta_data.input_attributes.push_back(MetaData::Attribute{ .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(glm::vec3) });
        return extension;

    }
    VolumeShader::MetaDataExtension VolumeShader::MetaDataExtension::createForFragmentShader(bool use_distance_field)
    {
        MetaDataExtension extension;
        extension._meta_data.input_attachments[kFrontFaceTextureBinding] = InputAttachment{ .binding = kFrontFaceTextureBinding };
        extension._meta_data.input_attachments[kBackFaceTextureBinding] = InputAttachment{ .binding = kBackFaceTextureBinding };
        if (use_distance_field)
        {
            extension._meta_data.samplers[kDistanceFieldBinding] = Sampler{ .binding = kDistanceFieldBinding, .update_frequency = MetaData::UpdateFrequency::PerDrawCall };
        }
        return extension;

    }
    void VolumeShader::MetaDataExtension::addSampler(uint32_t binding, Sampler sampler)
    {
        _meta_data.samplers[binding] = std::move(sampler);
    }
}