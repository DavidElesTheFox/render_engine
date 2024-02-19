#pragma once

#include <volk.h>

#include <vector>

#include <render_engine/assets/Material.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/LogicalDevice.h>
#include <render_engine/resources/GpuResourceSet.h>
#include <render_engine/resources/PushConstantsUpdater.h>
#include <render_engine/resources/UniformBinding.h>

namespace RenderEngine
{
    class Technique
    {
    public:

        Technique(LogicalDevice& logical_device,
                  const MaterialInstance* material,
                  TextureBindingMap&& subpass_textures,
                  GpuResourceSet&& constant_resources,
                  GpuResourceSet&& per_frame_resources,
                  GpuResourceSet&& per_draw_call_resources,
                  VkRenderPass render_pass,
                  uint32_t corresponding_subpass);
        ~Technique();
        const MaterialInstance& getMaterialInstance() const { return *_material_instance; }
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
            std::set<VkDescriptorSet> result;
            for (const UniformBinding* binding : (std::vector{ _per_frame_resources.getResources(), _per_draw_call_resources.getResources() }) | std::views::join)
            {
                result.insert(binding->getDescriptorSet(frame_number));
            }
            return std::vector<VkDescriptorSet>{ result.begin(), result.end() };
        }

        MaterialInstance::UpdateContext onFrameBegin(uint32_t frame_number, VkCommandBuffer command_buffer)
        {
            MaterialInstance::UpdateContext result(createPushConstantsUpdater(command_buffer),
                                                   *_material_instance);
            _material_instance->onFrameBegin(result, frame_number);
            return result;
        }
        void onDraw(MaterialInstance::UpdateContext& update_context, const MeshInstance* mesh_instance)
        {
            _material_instance->onDraw(update_context, mesh_instance);
        }

        std::ranges::input_range auto getUniformBindings() const { return (std::vector{ _per_frame_resources.getResources(), _per_draw_call_resources.getResources() }) | std::views::join; }

    private:
        void destroy();
        VkShaderStageFlags getPushConstantsUsageFlag() const;
        PushConstantsUpdater createPushConstantsUpdater(VkCommandBuffer command_buffer)
        {
            return PushConstantsUpdater{ _logical_device, command_buffer, _pipeline_layout, };
        }
        const MaterialInstance* _material_instance{ nullptr };

        TextureBindingMap _subpass_textures;
        GpuResourceSet _constant_resources;
        GpuResourceSet _per_frame_resources;
        GpuResourceSet _per_draw_call_resources;
        LogicalDevice& _logical_device;
        VkPipeline _pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout _pipeline_layout{ VK_NULL_HANDLE };
        uint32_t _corresponding_subpass{ 0 };
    };
}
