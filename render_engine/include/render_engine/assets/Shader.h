#pragma once

#include <volk.h>

#include <render_engine/LogicalDevice.h>

#include <filesystem>
#include <optional>
#include <ranges>
#include <span>
#include <unordered_map>
#include <vector>

namespace RenderEngine
{
    class ShaderModule;
    class Shader
    {
    public:

        struct MetaData
        {
            enum class UpdateFrequency
            {
                Unknown = 0,
                Constant,
                PerFrame,
                PerDrawCall
            };
            struct Attribute
            {
                uint32_t location{ 0 };
                VkFormat format{};
                uint32_t offset{ 0 };
            };

            struct UniformBuffer
            {
                int32_t binding{ -1 };
                int32_t size{ -1 };
                UpdateFrequency update_frequency{ UpdateFrequency::Unknown };
            };

            struct PushConstants
            {
                int32_t size{ -1 };
                int32_t offset{ 0 };
                VkShaderStageFlags shared_with{ 0 };
                UpdateFrequency update_frequency{ UpdateFrequency::Unknown };
            };

            struct Sampler
            {
                int32_t binding{ -1 };
                UpdateFrequency update_frequency{ UpdateFrequency::Unknown };
            };

            struct InputAttachment
            {
                int32_t binding{ -1 };
            };

            uint32_t attributes_stride{ 0 };
            std::vector<Attribute> input_attributes;
            std::unordered_map<int32_t, UniformBuffer> global_uniform_buffers;
            std::unordered_map<int32_t, Sampler> samplers;
            std::unordered_map<int32_t, InputAttachment> input_attachments;
            std::optional<PushConstants> push_constants{ std::nullopt };

        };
        Shader(const std::filesystem::path& spriv_path, MetaData meta_data)
            : _meta_data(std::move(meta_data))
        {
            readFromFile(spriv_path);
        }
        Shader(std::span<const uint32_t> spirv_code, MetaData meta_data)
            : _meta_data(std::move(meta_data))
            , _spirv_code(spirv_code.begin(), spirv_code.end())
        {}
        virtual ~Shader() = default;
        ShaderModule loadOn(LogicalDevice& logical_device) const;
        const MetaData& getMetaData() const
        {
            return _meta_data;
        }

    private:
        void readFromFile(const std::filesystem::path& spriv_path);
        MetaData _meta_data;
        std::vector<uint32_t> _spirv_code;
    };
}
