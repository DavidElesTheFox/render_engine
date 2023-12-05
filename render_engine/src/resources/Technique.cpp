#include <render_engine/resources/Technique.h>

#include <render_engine/resources/ShaderModule.h>

namespace RenderEngine
{
    Technique::Technique(VkDevice logical_device,
                         const MaterialInstance* material_instance,
                         std::vector<UniformBinding>&& uniform_buffers,
                         VkDescriptorSetLayout uniforms_layout,
                         VkRenderPass render_pass)
        try : _material_instance(material_instance)
        , _uniform_buffers(std::move(uniform_buffers))
        , _logical_device(logical_device)
        , _uniforms_layout(uniforms_layout)
    {
        const auto& material = _material_instance->getMaterial();
        auto vertex_shader = material.getVertexShader().loadOn(logical_device);
        auto fragment_shader = material.getFragmentShader().loadOn(logical_device);

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        if (_uniform_buffers.empty() == false)
        {
            descriptor_set_layouts.push_back(_uniforms_layout);
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = descriptor_set_layouts.size();
        pipelineLayoutInfo.pSetLayouts = descriptor_set_layouts.data();

        std::vector<VkPushConstantRange> push_constants_info_container{};

        for (const auto& [stage, push_constants] : material.getPushConstantsMetaData())
        {
            VkPushConstantRange push_constants_info;
            push_constants_info.offset = push_constants.offset;
            push_constants_info.size = push_constants.size;
            push_constants_info.stageFlags = stage;
            push_constants_info_container.push_back(push_constants_info);
        }

        pipelineLayoutInfo.pushConstantRangeCount = push_constants_info_container.size();
        pipelineLayoutInfo.pPushConstantRanges = push_constants_info_container.data();
        if (vkCreatePipelineLayout(logical_device, &pipelineLayoutInfo, nullptr, &_pipeline_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vertex_shader.getModule();
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_tage_info{};
        frag_shader_tage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_tage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_tage_info.module = fragment_shader.getModule();
        frag_shader_tage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_tage_info };

        std::vector<VkVertexInputBindingDescription> attribute_bindings;
        if (material.getVertexShader().getMetaData().attributes_stride > 0)
        {
            // TODO implement more binding
            VkVertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = material.getVertexShader().getMetaData().attributes_stride;
            // TODO add support of instanced rendering
            binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            attribute_bindings.emplace_back(std::move(binding_description));
        }
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
        for (const auto& attribute : material.getVertexShader().getMetaData().input_attributes)
        {
            VkVertexInputAttributeDescription description{};
            description.binding = 0;
            description.format = attribute.format;
            description.location = attribute.location;
            description.offset = attribute.offset;
            attribute_descriptions.emplace_back(std::move(description));
        }
        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = attribute_bindings.size();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions = attribute_bindings.data();
        vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;


        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = material.getRasterizationInfo().cull_mode;
        rasterizer.frontFace = material.getRasterizationInfo().front_face;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shader_stages;
        pipelineInfo.pVertexInputState = &vertex_input_info;
        pipelineInfo.pInputAssemblyState = &input_assembly;
        pipelineInfo.pViewportState = &viewport_state;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &color_blending;
        pipelineInfo.pDynamicState = &dynamic_state;
        pipelineInfo.layout = _pipeline_layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }
    catch (const std::exception&)
    {
        destroy();
    }
    Technique::~Technique()
    {
        destroy();
    }
    void Technique::destroy()
    {
        vkDestroyPipeline(_logical_device, _pipeline, nullptr);
        vkDestroyPipelineLayout(_logical_device, _pipeline_layout, nullptr);


        vkDestroyDescriptorSetLayout(_logical_device, _uniforms_layout, nullptr);
    }

    VkShaderStageFlags Technique::getPushConstantsUsageFlag() const
    {
        VkShaderStageFlags result{ 0 };
        if (_material_instance->getMaterial().getVertexShader().getMetaData().push_constants != std::nullopt)
        {
            result |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (_material_instance->getMaterial().getFragmentShader().getMetaData().push_constants != std::nullopt)
        {
            result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        return result;
    }
}
