#include <render_engine/renderers/ExampleRenderer.h>

#include <render_engine/DataTransferScheduler.h>
#include <render_engine/Device.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/resources/Texture.h>
#include <render_engine/window/Window.h>

#include <data_config.h>

#include <fstream>
#include <stdexcept>

namespace
{
    using namespace RenderEngine;
    VkVertexInputBindingDescription createBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ExampleRenderer::Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> createAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ExampleRenderer::Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ExampleRenderer::Vertex, color);

        return attributeDescriptions;
    }
    std::vector<char> readFile(std::string_view filename)
    {
        std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkRenderPass createRenderPass(const RenderTarget& render_target, LogicalDevice& logical_device)
    {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = render_target.getImageFormat();
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = render_target.getLoadOperation();
        color_attachment.storeOp = render_target.getStoreOperation();
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = render_target.getInitialLayout();
        color_attachment.finalLayout = render_target.getFinalLayout();

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &color_attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkRenderPass result;

        if (logical_device->vkCreateRenderPass(*logical_device, &renderPassInfo, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
        return result;
    }
    VkShaderModule createShaderModule(const std::vector<char>& code, LogicalDevice& logical_device)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (logical_device->vkCreateShaderModule(*logical_device, &create_info, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    std::pair<VkShaderModule, VkShaderModule> loadShaders(LogicalDevice& logical_device, std::string_view vert_shader_path, std::string_view frag_shader_path)
    {
        auto vert_shader_code = readFile(vert_shader_path);
        auto frag_shader_code = readFile(frag_shader_path);

        VkShaderModule vert_shader_module = createShaderModule(vert_shader_code, logical_device);
        VkShaderModule frag_shader_module = createShaderModule(frag_shader_code, logical_device);
        return { vert_shader_module, frag_shader_module };
    }

    VkDescriptorSetLayout createDescriptorSetLayout(LogicalDevice& logical_device)
    {
        VkDescriptorSetLayout result;
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (logical_device->vkCreateDescriptorSetLayout(*logical_device, &layoutInfo, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        return result;
    }

    VkDescriptorPool createDescriptorPool(LogicalDevice& logical_device, uint32_t backbuffer_size)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = backbuffer_size;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = backbuffer_size;

        VkDescriptorPool result;

        if (logical_device->vkCreateDescriptorPool(*logical_device, &poolInfo, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
        return result;
    }

    std::vector<VkDescriptorSet> createDescriptorSets(LogicalDevice& logical_device,
                                                      VkDescriptorPool pool,
                                                      VkDescriptorSetLayout layout,
                                                      uint32_t backbuffer_size,
                                                      std::vector<CoherentBuffer*> uniform_buffers)
    {
        std::vector<VkDescriptorSetLayout> layouts(backbuffer_size, layout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(backbuffer_size);
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> descriptor_sets;
        descriptor_sets.resize(backbuffer_size);
        if (logical_device->vkAllocateDescriptorSets(*logical_device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < backbuffer_size; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniform_buffers[i]->getBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(ExampleRenderer::ColorOffset);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptor_sets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            logical_device->vkUpdateDescriptorSets(*logical_device, 1, &descriptorWrite, 0, nullptr);
        }
        return descriptor_sets;
    }

    VkPipelineLayout createPipelineLayout(LogicalDevice& logical_device,
                                          VkDescriptorSetLayout descriptor_set_layout,
                                          VkShaderModule vertShaderModule,
                                          VkShaderModule fragShaderModule)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout;

        VkPipelineLayout pipeline_layout;
        if (logical_device->vkCreatePipelineLayout(*logical_device, &pipelineLayoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS)
        {
            logical_device->vkDestroyShaderModule(*logical_device, fragShaderModule, nullptr);
            logical_device->vkDestroyShaderModule(*logical_device, vertShaderModule, nullptr);
            throw std::runtime_error("failed to create pipeline layout!");
        }
        return pipeline_layout;
    }

    VkPipeline createGraphicsPipeline(LogicalDevice& logical_device, VkRenderPass render_pass, VkPipelineLayout pipeline_layout, VkShaderModule vert_shader, VkShaderModule frag_shader)
    {
        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_tage_info{};
        frag_shader_tage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_tage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_tage_info.module = frag_shader;
        frag_shader_tage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_tage_info };

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = 0;
        vertex_input_info.vertexAttributeDescriptionCount = 0;

        auto binding_description = createBindingDescription();
        auto attribute_descriptions = createAttributeDescriptions();

        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions = &binding_description;
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
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
        pipelineInfo.layout = pipeline_layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        VkPipeline graphics_pipeline;
        if (logical_device->vkCreateGraphicsPipelines(*logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        return graphics_pipeline;
    }

}

namespace RenderEngine
{
    ExampleRenderer::ExampleRenderer(IRenderEngine& render_engine,
                                     const RenderTarget& render_target,
                                     uint32_t back_buffer_size)
        : _render_engine(render_engine)
    {
        auto& logical_device = getLogicalDevice();
        auto [vert_shader, frag_shader] = loadShaders(logical_device, BASE_VERT_SHADER, BASE_FRAG_SHADER);

        try
        {
            _descriptor_set_layout = createDescriptorSetLayout(logical_device);
            _descriptor_pool = createDescriptorPool(logical_device, back_buffer_size);
            _pipeline_layout = createPipelineLayout(logical_device, _descriptor_set_layout, vert_shader, frag_shader);
            _render_pass = createRenderPass(render_target, logical_device);
            _pipeline = createGraphicsPipeline(logical_device, _render_pass, _pipeline_layout, vert_shader, frag_shader);

            _back_buffer.resize(back_buffer_size);
            _render_area.offset = { 0, 0 };
            _render_area.extent = render_target.getExtent();
            createFrameBuffers(render_target);
            createCommandBuffer();
            logical_device->vkDestroyShaderModule(*logical_device, vert_shader, nullptr);
            logical_device->vkDestroyShaderModule(*logical_device, frag_shader, nullptr);
        }
        catch (const std::runtime_error&)
        {
            for (auto framebuffer : _frame_buffers)
            {
                logical_device->vkDestroyFramebuffer(*logical_device, framebuffer, nullptr);
            }
            logical_device->vkDestroyDescriptorSetLayout(*logical_device, _descriptor_set_layout, nullptr);
            logical_device->vkDestroyPipelineLayout(*logical_device, _pipeline_layout, nullptr);
            logical_device->vkDestroyRenderPass(*logical_device, _render_pass, nullptr);
            logical_device->vkDestroyPipeline(*logical_device, _pipeline, nullptr);

            logical_device->vkDestroyShaderModule(*logical_device, vert_shader, nullptr);
            logical_device->vkDestroyShaderModule(*logical_device, frag_shader, nullptr);
            throw;
        }
    }


    void ExampleRenderer::init(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indicies)
    {
        {
            VkDeviceSize size = sizeof(Vertex) * vertices.size();
            _vertex_buffer = _render_engine.getGpuResourceManager().createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);

            _render_engine.getDevice().getDataTransferContext().getScheduler().upload(_vertex_buffer.get(),
                                                                                      std::span(vertices),
                                                                                      _render_engine.getTransferEngine().getCommandBufferFactory(),
                                                                                      _vertex_buffer->getResourceState().clone());
        }
        {
            VkDeviceSize size = sizeof(uint16_t) * indicies.size();
            _index_buffer = _render_engine.getGpuResourceManager().createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            _render_engine.getDevice().getDataTransferContext().getScheduler().upload(_index_buffer.get(),
                                                                                      std::span(indicies),
                                                                                      _render_engine.getTransferEngine().getCommandBufferFactory(),
                                                                                      _vertex_buffer->getResourceState().clone());
        }

        std::vector<CoherentBuffer*> created_buffers;
        for (auto& frame_data : _back_buffer)
        {
            frame_data.color_offset = _render_engine.getGpuResourceManager().createUniformBuffer(sizeof(ColorOffset));
            created_buffers.push_back(frame_data.color_offset.get());
        }

        auto descriptor_sets = createDescriptorSets(_render_engine.getDevice().getLogicalDevice(),
                                                    _descriptor_pool,
                                                    _descriptor_set_layout,
                                                    static_cast<uint32_t>(_back_buffer.size()),
                                                    created_buffers);

        for (size_t i = 0; i < descriptor_sets.size(); ++i)
        {
            _back_buffer[i].descriptor_set = descriptor_sets[i];
        }
    }

    void ExampleRenderer::draw(const VkFramebuffer& frame_buffer, FrameData& frame_data)
    {
        frame_data.color_offset->upload(std::span(&_color_offset, 1));

        getLogicalDevice()->vkResetCommandBuffer(frame_data.command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (getLogicalDevice()->vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = _render_pass;
        render_pass_info.framebuffer = frame_buffer;
        render_pass_info.renderArea = _render_area;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        getLogicalDevice()->vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        getLogicalDevice()->vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)_render_area.extent.width;
        viewport.height = (float)_render_area.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        getLogicalDevice()->vkCmdSetViewport(frame_data.command_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = _render_area.extent;
        getLogicalDevice()->vkCmdSetScissor(frame_data.command_buffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { _vertex_buffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        getLogicalDevice()->vkCmdBindVertexBuffers(frame_data.command_buffer, 0, 1, vertexBuffers, offsets);
        getLogicalDevice()->vkCmdBindIndexBuffer(frame_data.command_buffer, _index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
        getLogicalDevice()->vkCmdBindDescriptorSets(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &frame_data.descriptor_set, 0, nullptr);

        getLogicalDevice()->vkCmdDrawIndexed(frame_data.command_buffer, static_cast<uint32_t>(_index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);

        getLogicalDevice()->vkCmdEndRenderPass(frame_data.command_buffer);

        if (getLogicalDevice()->vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    ExampleRenderer::~ExampleRenderer()
    {
        auto& logical_device = getLogicalDevice();

        resetFrameBuffers();

        logical_device->vkDestroyDescriptorPool(*logical_device, _descriptor_pool, nullptr);
        logical_device->vkDestroyDescriptorSetLayout(*logical_device, _descriptor_set_layout, nullptr);

        logical_device->vkDestroyPipelineLayout(*logical_device, _pipeline_layout, nullptr);
        logical_device->vkDestroyRenderPass(*logical_device, _render_pass, nullptr);
        logical_device->vkDestroyPipeline(*logical_device, _pipeline, nullptr);
    }
    void ExampleRenderer::resetFrameBuffers()
    {
        auto& logical_device = getLogicalDevice();

        for (auto framebuffer : _frame_buffers)
        {
            logical_device->vkDestroyFramebuffer(*logical_device, framebuffer, nullptr);
        }
    }
    void ExampleRenderer::beforeReinit()
    {
        resetFrameBuffers();
    }
    void ExampleRenderer::finalizeReinit(const RenderTarget& render_target)
    {
        createFrameBuffers(render_target);
        _render_area.offset = { 0, 0 };
        _render_area.extent = render_target.getExtent();
    }
    void ExampleRenderer::createFrameBuffers(const RenderTarget& render_target)
    {
        auto& logical_device = getLogicalDevice();

        _frame_buffers.resize(render_target.getTexturesCount());
        for (uint32_t i = 0; i < _frame_buffers.size(); ++i)
        {
            if (createFrameBuffer(render_target, i) == false)
            {
                for (uint32_t j = 0; j < i; ++j)
                {
                    logical_device->vkDestroyFramebuffer(*logical_device, _frame_buffers[j], nullptr);
                }
                _frame_buffers.clear();
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    bool ExampleRenderer::createFrameBuffer(const RenderTarget& render_target, uint32_t frame_buffer_index)
    {
        auto& logical_device = getLogicalDevice();

        VkImageView attachments[] = {
                render_target.getTextureView(frame_buffer_index)->getImageView()
        };
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = render_target.getWidth();
        framebuffer_info.height = render_target.getHeight();
        framebuffer_info.layers = 1;

        return logical_device->vkCreateFramebuffer(*logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) == VK_SUCCESS;
    }

    void ExampleRenderer::createCommandBuffer()
    {
        for (uint32_t i = 0; i < _back_buffer.size(); ++i)
        {
            // TODO add threading information or remove function
            VkCommandBuffer command_buffer = _render_engine.getCommandBufferContext().getFactory()->createCommandBuffer(i, 0);

            _back_buffer[i].command_buffer = command_buffer;
        }
    }
}