#pragma once

#include <volk.h>

#include <vector>

#include <render_engine/assets/Material.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/resources/PushConstantsUpdater.h>
#include <render_engine/resources/UniformBinding.h>

namespace RenderEngine
{
    class Technique
    {
    public:

        Technique(VkDevice logical_device,
                  const MaterialInstance* material,
                  std::vector<UniformBinding>&& uniform_buffers,
                  VkDescriptorSetLayout uniforms_layout,
                  VkRenderPass render_pass);
        ~Technique();
        VkPipeline getPipeline()
        {
            return _pipeline;
        }
        VkPipelineLayout getPipelineLayout()
        {
            return _pipeline_layout;
        }
        std::vector<VkDescriptorSet> collectDescriptorSets(size_t frame_number)
        {
            std::vector<VkDescriptorSet> result;
            for (auto& binding : _uniform_buffers)
            {
                result.push_back(binding.getDescriptorSet(frame_number));
            }
            return result;
        }
        VkDescriptorSetLayout getDescriptorSetLayout() const { return _uniforms_layout; }
        PushConstantsUpdater createPushConstantsUpdater(VkCommandBuffer command_buffer)
        {
            return PushConstantsUpdater{ command_buffer, _pipeline_layout };
        }
        void updateGlobalUniformBuffer(uint32_t frame_number)
        {
            _material_instance->updateGlobalUniformBuffer(_uniform_buffers, frame_number);
        }

        void updateGlobalPushConstants(PushConstantsUpdater& updater)
        {
            _material_instance->updateGlobalPushConstants(updater);
        }

        const std::vector<UniformBinding>& getUniformBindings() const { return _uniform_buffers; }

    private:
        void destroy();
        VkShaderStageFlags getPushConstantsUsageFlag() const;

        const MaterialInstance* _material_instance{ nullptr };

        std::vector<UniformBinding> _uniform_buffers;
        VkDevice _logical_device;
        VkPipeline _pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout _pipeline_layout{ VK_NULL_HANDLE };
        VkDescriptorSetLayout _uniforms_layout{ VK_NULL_HANDLE };
    };
}
