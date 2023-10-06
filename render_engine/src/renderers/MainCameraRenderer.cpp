#include "render_engine/renderers/MainCameraRenderer.h"

#include "render_engine/window/Window.h"
#include "render_engine/RenderEngine.h"
#include <fstream>
#include <ranges>
#include <volk.h>
namespace
{
	std::vector<char> readFile(std::string_view filename) {
		std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

		if (!file.is_open()) {

			throw std::runtime_error("failed to open file " + std::string{ filename });
		}

		size_t fileSize = file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
}
namespace RenderEngine
{

	Shader::LoadedShader::~LoadedShader()
	{
		if (_device_loaded_on == VK_NULL_HANDLE)
		{
			return;
		}
		vkDestroyShaderModule(_device_loaded_on, _module, nullptr);
	}

	Shader::LoadedShader Shader::loadOn(VkDevice logical_device) const
	{
		const std::vector<char> code = readFile(_spirv_path.string());
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module;
		if (vkCreateShaderModule(logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return LoadedShader(shader_module, logical_device);
	}
	//////////////////////////////////////////////

	Material::Material(Shader verted_shader,
		Shader fragment_shader,
		std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> buffer_update_callback,
		std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> vertex_buffer_create_callback,
		uint32_t id)
		: _vertex_shader(verted_shader)
		, _fragment_shader(fragment_shader)
		, _id{ id }
		, _buffer_update_callback(buffer_update_callback)
		, _vertex_buffer_create_callback(vertex_buffer_create_callback)
	{
	}

	std::unique_ptr<Technique> Material::createTechnique(VkDevice logical_device,
		GpuResourceManager& gpu_resource_manager,
		VkRenderPass render_pass) const
	{
		std::unordered_map<int32_t, VkShaderStageFlags> binding_usage;
		for (const auto& binding : _vertex_shader.metaData().uniform_buffers | std::views::keys)
		{
			binding_usage[binding] |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		for (const auto& binding : _fragment_shader.metaData().uniform_buffers | std::views::keys)
		{
			binding_usage[binding] |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		std::vector<UniformBinding> vertex_bindings = createUniformBindings(gpu_resource_manager,
			_vertex_shader.metaData().uniform_buffers
			| std::views::values
			| std::views::filter([&](const auto& ubo) {return binding_usage[ubo.binding] == VK_SHADER_STAGE_VERTEX_BIT; }),
			VK_SHADER_STAGE_VERTEX_BIT);
		std::vector<UniformBinding> fragment_bindings = createUniformBindings(gpu_resource_manager,
			_fragment_shader.metaData().uniform_buffers
			| std::views::values
			| std::views::filter([&](const auto& ubo) {return binding_usage[ubo.binding] == VK_SHADER_STAGE_FRAGMENT_BIT; }),
			VK_SHADER_STAGE_FRAGMENT_BIT);
		/*std::vector<UniformBinding> general_bindings = createUniformBindings(gpu_resource_manager,
			_fragment_shader.metaData().uniform_buffers
			| std::views::values
			| std::views::filter([&](const auto& ubo) {return binding_usage[ubo.binding] != 0; }),
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);*/
		std::vector<UniformBinding> all_bindings;
		std::ranges::move(vertex_bindings, std::back_inserter(all_bindings));
		std::ranges::move(fragment_bindings, std::back_inserter(all_bindings));
		//std::ranges::move(general_bindings, std::back_inserter(all_bindings));
		return std::make_unique<Technique>(logical_device,
			this,
			std::move(all_bindings),
			render_pass);
	}

	std::vector<UniformBinding> Material::createUniformBindings(GpuResourceManager& gpu_resource_manager,
		std::ranges::input_range auto&& uniforms_data,
		VkShaderStageFlags shader_stage) const
	{

		if (std::ranges::empty(uniforms_data))
		{
			return {};
		}
		uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();
		auto descriptor_pool = gpu_resource_manager.descriptorPool();
		std::vector<UniformBinding> result;
		// Create resources' layout
		int32_t num_of_bindings = std::distance(uniforms_data.begin(), uniforms_data.end());

		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = num_of_bindings;

		std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
		for (const auto& buffer_meta_data : uniforms_data)
		{
			VkDescriptorSetLayoutBinding set_layout_binding = {};

			set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			set_layout_binding.binding = buffer_meta_data.binding;
			set_layout_binding.stageFlags = shader_stage;
			set_layout_binding.descriptorCount = 1;
			layout_bindings.emplace_back(std::move(set_layout_binding));

		}
		layout_info.pBindings = layout_bindings.data();

		VkDescriptorSetLayout descriptor_set_layout;
		if (vkCreateDescriptorSetLayout(gpu_resource_manager.getLogicalDevice(), &layout_info, nullptr, &descriptor_set_layout)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("Cannot crate descriptor set layout for a shader of material: " + std::to_string(_id));
		}
		// create resources' binding
		std::vector<VkDescriptorSetLayout> layouts(back_buffer_size, descriptor_set_layout);
		std::vector<BackBuffer<UniformBinding::FrameData>> back_buffers;

		VkDescriptorSetAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool;
		alloc_info.descriptorSetCount = static_cast<uint32_t>(back_buffer_size);
		alloc_info.pSetLayouts = layouts.data();

		std::vector<VkDescriptorSet> descriptor_sets;
		descriptor_sets.resize(back_buffer_size);
		if (vkAllocateDescriptorSets(gpu_resource_manager.getLogicalDevice(), &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
			vkDestroyDescriptorSetLayout(gpu_resource_manager.getLogicalDevice(), descriptor_set_layout, nullptr);
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		std::vector<VkWriteDescriptorSet> descriptor_writers;
		// make buffer assignment
		for (const auto& buffer_meta_data : uniforms_data)
		{
			BackBuffer<UniformBinding::FrameData> back_buffer(back_buffer_size);

			for (size_t i = 0; i < back_buffer_size; ++i) {
				back_buffer[i]._buffer = gpu_resource_manager.createUniformBuffer(buffer_meta_data.size);
				back_buffer[i]._descriptor_set = descriptor_sets[i];

				VkDescriptorBufferInfo buffer_info{};
				buffer_info.buffer = back_buffer[i]._buffer->getBuffer();
				buffer_info.offset = 0;
				buffer_info.range = buffer_meta_data.size;

				VkWriteDescriptorSet writer{};
				writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writer.dstSet = descriptor_sets[i];
				writer.dstBinding = buffer_meta_data.binding;
				writer.dstArrayElement = 0;
				writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writer.descriptorCount = 1;
				writer.pBufferInfo = &buffer_info;

				descriptor_writers.emplace_back(std::move(writer));
			}
			back_buffers.emplace_back(std::move(back_buffer));
		}
		vkUpdateDescriptorSets(gpu_resource_manager.getLogicalDevice(),
			descriptor_writers.size(), descriptor_writers.data(), 0, nullptr);
		std::vector<UniformBinding> uniform_bindings;
		for (auto& back_buffer : back_buffers)
		{
			uniform_bindings.emplace_back(descriptor_set_layout, std::move(back_buffer), gpu_resource_manager.getLogicalDevice());
		}
		return uniform_bindings;
	}

	UniformBinding::~UniformBinding()
	{
		if (_logical_device == VK_NULL_HANDLE)
		{
			return;
		}
		vkDestroyDescriptorSetLayout(_logical_device, _descriptor_set_layout, nullptr);

	}

	Technique::Technique(VkDevice logical_device,
		const Material* material,
		std::vector<UniformBinding>&& uniform_buffers,
		VkRenderPass render_pass)
		try : _material(material)
		, _uniform_buffers(std::move(uniform_buffers))
		, _logical_device(logical_device)
	{
		auto vertex_shader = material->vertexShader().loadOn(logical_device);
		auto fragment_shader = material->fragmentShader().loadOn(logical_device);

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
		if (_uniform_buffers.empty() == false)
		{
			descriptor_set_layouts.push_back(_uniform_buffers[0].descriptorSetLayout());
		}
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.setLayoutCount = descriptor_set_layouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptor_set_layouts.data();


		if (vkCreatePipelineLayout(logical_device, &pipelineLayoutInfo, nullptr, &_pipeline_layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vertex_shader.module();
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_tage_info{};
		frag_shader_tage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_tage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_tage_info.module = fragment_shader.module();
		frag_shader_tage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_tage_info };


		// TODO implement more binding
		VkVertexInputBindingDescription binding_description{};
		binding_description.binding = 0;
		binding_description.stride = material->vertexShader().metaData().stride;
		// TODO add support of instanced rendering
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
		for (const auto& attribute : material->vertexShader().metaData().input_attributes)
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
		pipelineInfo.layout = _pipeline_layout;
		pipelineInfo.renderPass = render_pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
	catch (const std::exception&)
	{
		vkDestroyPipeline(_logical_device, _pipeline, nullptr);
		vkDestroyPipelineLayout(_logical_device, _pipeline_layout, nullptr);
	}
	Technique::~Technique()
	{
		vkDestroyPipeline(_logical_device, _pipeline, nullptr);
		vkDestroyPipelineLayout(_logical_device, _pipeline_layout, nullptr);
	}


	//////////////////////////////////////////////

	ForwardRenderer::ForwardRenderer(Window& parent,
		const SwapChain& swap_chain,
		bool last_renderer,
		std::vector<Material*> supported_materials)
		try : _window(parent)
		, _back_buffer(parent.gpuResourceManager().getBackBufferSize())
	{

		_render_area.offset = { 0, 0 };
		_render_area.extent = swap_chain.getDetails().extent;

		VkAttachmentDescription color_attachment{};
		color_attachment.format = swap_chain.getDetails().image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = last_renderer ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

		auto logical_device = _window.getRenderEngine().getLogicalDevice();
		if (vkCreateRenderPass(logical_device, &renderPassInfo, nullptr, &_render_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

		createFrameBuffers(swap_chain);
		createCommandPool(_window.getRenderQueueFamily());
		createCommandBuffer();


		for (const auto* material : supported_materials)
		{
			std::unique_ptr<Technique> technique = material->createTechnique(logical_device, _window.gpuResourceManager(), _render_pass);
			_technique_map.insert(std::make_pair(material->id(), std::move(technique)));
		}
	}
	catch (const std::exception&)
	{
		auto logical_device = parent.getRenderEngine().getLogicalDevice();
		for (uint32_t i = 0; i < _frame_buffers.size(); ++i) {
			vkDestroyFramebuffer(logical_device, _frame_buffers[i], nullptr);
		}
		_frame_buffers.clear();
		vkDestroyRenderPass(logical_device, _render_pass, nullptr);
		vkDestroyCommandPool(logical_device, _command_pool, nullptr);
	}

	ForwardRenderer::~ForwardRenderer()
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();
		resetFrameBuffers();

		vkDestroyRenderPass(logical_device, _render_pass, nullptr);
		vkDestroyCommandPool(logical_device, _command_pool, nullptr);

	}
	void ForwardRenderer::resetFrameBuffers()
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		for (auto framebuffer : _frame_buffers) {
			vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
		}
	}
	void ForwardRenderer::beforeReinit()
	{
		resetFrameBuffers();
	}
	void ForwardRenderer::finalizeReinit(const SwapChain& swap_chain)
	{
		createFrameBuffers(swap_chain);
		_render_area.offset = { 0, 0 };
		_render_area.extent = swap_chain.getDetails().extent;
	}
	void ForwardRenderer::addMesh(Mesh* mesh, int32_t priority)
	{
		const Geometry& geometry = mesh->geometry();

		const Shader::MetaData& vertex_shader_meta_data = mesh->material().vertexShader().metaData();
		MeshBuffers mesh_buffers;
		if (geometry.positions.empty() == false)
		{
			std::vector vertex_buffer = mesh->createVertexBuffer();
			mesh_buffers.vertex_buffer = _window.gpuResourceManager().createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				vertex_buffer.size());
			mesh_buffers.vertex_buffer->uploadUnmapped(std::span{ vertex_buffer.data(), vertex_buffer.size() }, _window.getRenderQueue(), _command_pool);

		}
		if (geometry.indexes.empty() == false)
		{
			mesh_buffers.index_buffer = _window.gpuResourceManager().createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				geometry.indexes.size() * sizeof(int16_t));
			mesh_buffers.index_buffer->uploadUnmapped(std::span{ geometry.indexes.data(), geometry.indexes.size() }, _window.getRenderQueue(), _command_pool);
		}
		_meshes[priority][mesh->material().id()].mesh_buffers[mesh] = std::move(mesh_buffers);
		_meshes[priority][mesh->material().id()].technique = _technique_map[mesh->material().id()].get();
	}

	void ForwardRenderer::draw(uint32_t swap_chain_image_index, uint32_t frame_number)
	{
		FrameData& frame_data = _back_buffer[frame_number];

		//frame_data.color_offset->uploadMapped(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&_color_offset), sizeof(ColorOffset)));

		vkResetCommandBuffer(frame_data.command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _render_pass;
		render_pass_info.framebuffer = _frame_buffers[swap_chain_image_index];
		render_pass_info.renderArea = _render_area;

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clearColor;

		vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		for (auto& mesh_group_container : _meshes | std::views::values)
		{
			for (auto& mesh_group : mesh_group_container | std::views::values)
			{
				mesh_group.technique->update(frame_number);

				vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_group.technique->getPipeline());

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
				auto descriptor_sets = mesh_group.technique->collectDescriptorSets(frame_number);

				vkCmdBindDescriptorSets(frame_data.command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					mesh_group.technique->getPipelineLayout(),
					0,
					descriptor_sets.size(),
					descriptor_sets.data(), 0, nullptr);

				for (auto& [mesh, mesh_buffers] : mesh_group.mesh_buffers)
				{

					VkBuffer vertexBuffers[] = { mesh_buffers.vertex_buffer->getBuffer() };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(frame_data.command_buffer, 0, 1, vertexBuffers, offsets);
					vkCmdBindIndexBuffer(frame_data.command_buffer, mesh_buffers.index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);


					vkCmdDrawIndexed(frame_data.command_buffer, static_cast<uint32_t>(mesh_buffers.index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);
				}
			}
		}
		vkCmdEndRenderPass(frame_data.command_buffer);

		if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void ForwardRenderer::createFrameBuffers(const SwapChain& swap_chain)
	{
		auto logical_device = _window.getRenderEngine().getLogicalDevice();

		_frame_buffers.resize(swap_chain.getDetails().image_views.size(), VK_NULL_HANDLE);
		for (uint32_t i = 0; i < swap_chain.getDetails().image_views.size(); ++i)
		{
			createFrameBuffer(swap_chain, i);
		}
	}
	void ForwardRenderer::createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index)
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

		if (vkCreateFramebuffer(logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
	void ForwardRenderer::createCommandPool(uint32_t render_queue_family)
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
	void ForwardRenderer::createCommandBuffer()
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