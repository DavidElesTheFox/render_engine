#include <render_engine/Drawer.h>
#include <render_engine/Window.h>
#include <render_engine/RenderEngine.h>

#include <data_config.h>

#include <fstream>
#include <stdexcept>

namespace
{
	using namespace RenderEngine;
	VkVertexInputBindingDescription createBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Drawer::Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> createAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Drawer::Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Drawer::Vertex, color);

		return attributeDescriptions;
	}
	std::vector<char> readFile(std::string_view filename) {
		std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	VkRenderPass createRenderPass(const SwapChain& swap_chain, VkDevice logical_device) {
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swap_chain.getDetails().image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

		if (vkCreateRenderPass(logical_device, &renderPassInfo, nullptr, &result) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
		return result;
	}
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice logical_device) {
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(logical_device, &create_info, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	std::pair<VkShaderModule, VkShaderModule> loadShaders(VkDevice logical_device, std::string_view vert_shader_path, std::string_view frag_shader_path)
	{
		auto vert_shader_code = readFile(vert_shader_path);
		auto frag_shader_code = readFile(frag_shader_path);

		VkShaderModule vert_shader_module = createShaderModule(vert_shader_code, logical_device);
		VkShaderModule frag_shader_module = createShaderModule(frag_shader_code, logical_device);
		return { vert_shader_module, frag_shader_module };
	}

	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logical_device)
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

		if (vkCreateDescriptorSetLayout(logical_device, &layoutInfo, nullptr, &result) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
		return result;
	}

	VkDescriptorPool createDescriptorPool(VkDevice logical_device, uint32_t backbuffer_size) {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = backbuffer_size;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = backbuffer_size;

		VkDescriptorPool result;

		if (vkCreateDescriptorPool(logical_device, &poolInfo, nullptr, &result) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
		return result;
	}

	std::vector<VkDescriptorSet> createDescriptorSets(VkDevice logical_device,
		VkDescriptorPool pool,
		VkDescriptorSetLayout layout,
		uint32_t backbuffer_size,
		std::vector<Buffer*> uniform_buffers) {
		std::vector<VkDescriptorSetLayout> layouts(backbuffer_size, layout);
		VkDescriptorSetAllocateInfo allocInfo{};
		VkDescriptorSet result;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(backbuffer_size);
		allocInfo.pSetLayouts = layouts.data();

		std::vector<VkDescriptorSet> descriptor_sets;
		descriptor_sets.resize(backbuffer_size);
		if (vkAllocateDescriptorSets(logical_device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < backbuffer_size; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniform_buffers[i]->getBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(Drawer::ColorOffset);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptor_sets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(logical_device, 1, &descriptorWrite, 0, nullptr);
		}
		return descriptor_sets;
	}

	VkPipelineLayout createPipelineLayout(VkDevice logical_device,
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
		if (vkCreatePipelineLayout(logical_device, &pipelineLayoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS) {
			vkDestroyShaderModule(logical_device, fragShaderModule, nullptr);
			vkDestroyShaderModule(logical_device, vertShaderModule, nullptr);
			throw std::runtime_error("failed to create pipeline layout!");
		}
		return pipeline_layout;
	}

	VkPipeline createGraphicsPipeline(VkDevice logical_device, VkRenderPass render_pass, VkPipelineLayout pipeline_layout, VkShaderModule vert_shader, VkShaderModule frag_shader) {
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
		if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
		return graphics_pipeline;
	}

}

namespace RenderEngine
{
	Drawer::Drawer(Window& window,
		const SwapChain& swap_chain,
		uint32_t back_buffer_size)
		: _window(window)
	{
		auto logical_device = window.getRenderEngine().getLogicalDevice();
		auto [vert_shader, frag_shader] = loadShaders(logical_device, BASE_VERT_SHADER, BASE_FRAG_SHADER);

		try
		{
			_descriptor_set_layout = createDescriptorSetLayout(logical_device);
			_descriptor_pool = createDescriptorPool(logical_device, back_buffer_size);
			_pipeline_layout = createPipelineLayout(logical_device, _descriptor_set_layout, vert_shader, frag_shader);
			_render_pass = createRenderPass(swap_chain, logical_device);
			_pipeline = createGraphicsPipeline(logical_device, _render_pass, _pipeline_layout, vert_shader, frag_shader);

			_back_buffer.resize(back_buffer_size);
			_render_area.offset = { 0, 0 };
			_render_area.extent = swap_chain.getDetails().extent;
			createFrameBuffers(swap_chain);
			createCommandPool(window.getRenderQueueFamily());
			createCommandBuffer();
			vkDestroyShaderModule(logical_device, vert_shader, nullptr);
			vkDestroyShaderModule(logical_device, frag_shader, nullptr);
		}
		catch (const std::runtime_error&)
		{
			auto logical_device = window.getRenderEngine().getLogicalDevice();
			vkDestroyCommandPool(logical_device, _command_pool, nullptr);

			for (auto framebuffer : _frame_buffers) {
				vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
			}
			vkDestroyDescriptorSetLayout(logical_device, _descriptor_set_layout, nullptr);
			vkDestroyPipelineLayout(logical_device, _pipeline_layout, nullptr);
			vkDestroyRenderPass(logical_device, _render_pass, nullptr);
			vkDestroyPipeline(logical_device, _pipeline, nullptr);

			vkDestroyShaderModule(logical_device, vert_shader, nullptr);
			vkDestroyShaderModule(logical_device, frag_shader, nullptr);
			throw;
		}
	}


	void Drawer::init(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indicies)
	{
		{
			VkDeviceSize size = sizeof(Vertex) * vertices.size();
			_vertex_buffer = _window.getRenderEngine().createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
			_vertex_buffer->uploadUnmapped(std::span<const Vertex>{vertices.data(), vertices.size()}, _window.getRenderQueue(), _command_pool);
		}
		{
			VkDeviceSize size = sizeof(uint16_t) * indicies.size();
			_index_buffer = _window.getRenderEngine().createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
			_index_buffer->uploadUnmapped(std::span<const uint16_t>{indicies.data(), indicies.size()}, _window.getRenderQueue(), _command_pool);
		}

		std::vector<Buffer*> created_buffers;
		for (auto& frame_data : _back_buffer)
		{
			frame_data.color_offset = _window.getRenderEngine().createUniformBuffer(sizeof(ColorOffset));
			created_buffers.push_back(frame_data.color_offset.get());
		}

		auto descriptor_sets = createDescriptorSets(_window.getRenderEngine().getLogicalDevice(),
			_descriptor_pool,
			_descriptor_set_layout,
			_back_buffer.size(),
			created_buffers);

		for (size_t i = 0; i < descriptor_sets.size(); ++i)
		{
			_back_buffer[i].descriptor_set = descriptor_sets[i];
		}
	}

	void Drawer::draw(const VkFramebuffer& frame_buffer, uint32_t frame_number)
	{
		FrameData& frame_data = getFrameData(frame_number);

		frame_data.color_offset->uploadMapped(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&_color_offset), sizeof(ColorOffset)));

		vkResetCommandBuffer(frame_data.command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS) {
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

		vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)_render_area.extent.width;
		viewport.height = (float)_render_area.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(frame_data.command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = _render_area.extent;
		vkCmdSetScissor(frame_data.command_buffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { _vertex_buffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(frame_data.command_buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(frame_data.command_buffer, _index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &frame_data.descriptor_set, 0, nullptr);

		vkCmdDrawIndexed(frame_data.command_buffer, static_cast<uint32_t>(_index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);

		vkCmdEndRenderPass(frame_data.command_buffer);

		if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	Drawer::ReinitializationCommand Drawer::reinit()
	{
		return ReinitializationCommand(*this);
	}

	Drawer::~Drawer()
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();
		vkDestroyCommandPool(logical_device, _command_pool, nullptr);

		resetFrameBuffers();

		vkDestroyDescriptorPool(logical_device, _descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(logical_device, _descriptor_set_layout, nullptr);

		vkDestroyPipelineLayout(logical_device, _pipeline_layout, nullptr);
		vkDestroyRenderPass(logical_device, _render_pass, nullptr);
		vkDestroyPipeline(logical_device, _pipeline, nullptr);
	}
	void Drawer::resetFrameBuffers()
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		for (auto framebuffer : _frame_buffers) {
			vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
		}
	}
	void Drawer::createFrameBuffers(const SwapChain& swap_chain)
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		_frame_buffers.resize(swap_chain.getDetails().image_views.size());
		for (uint32_t i = 0; i < swap_chain.getDetails().image_views.size(); ++i)
		{
			if (createFrameBuffer(swap_chain, i) == false)
			{
				for (uint32_t j = 0; j < i; ++j) {
					vkDestroyFramebuffer(logical_device, _frame_buffers[j], nullptr);
				}
				_frame_buffers.clear();
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}
	bool Drawer::createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index)
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		VkImageView attachments[] = {
				swap_chain.getDetails().image_views[frame_buffer_index]
		};
		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = _render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = swap_chain.getDetails().extent.width;
		framebuffer_info.height = swap_chain.getDetails().extent.height;
		framebuffer_info.layers = 1;

		return vkCreateFramebuffer(logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) == VK_SUCCESS;
	}
	void Drawer::createCommandPool(uint32_t render_queue_family)
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = render_queue_family;
		if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	void Drawer::createCommandBuffer()
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		for (FrameData& frame_data : _back_buffer)
		{
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = _command_pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			if (vkAllocateCommandBuffers(logical_device, &allocInfo, &frame_data.command_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}
		}
	}
}