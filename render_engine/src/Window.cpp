#include <render_engine/Window.h>

#include <data_config.h>

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <limits>
#include <fstream>
namespace
{
	using namespace RenderEngine;
	static std::vector<char> readFile(std::string_view filename) {
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

	VkRenderPass createRenderPass(SwapChain& swap_chain, VkDevice logical_device) {
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

	VkPipelineLayout createPipelineLayout(VkDevice logical_device, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
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
	Window::Window(VkDevice logical_device, GLFWwindow* window, std::unique_ptr<SwapChain> swap_chain, VkQueue render_queue, VkQueue present_queue, uint32_t render_queue_family)
		: _render_queue{ render_queue }
		, _present_queue{ present_queue }
		, _window(window)
		, _swap_chain(std::move(swap_chain))
		, _logical_device(logical_device)
		, _render_queue_family(render_queue_family)
	{
		initSynchronizationObjects();
	}

	void Window::update()
	{
		if (_closed)
		{
			return;
		}
		handleEvents();
		present();

	}

	void Window::registerDrawer()
	{
		auto [vert_shader, frag_shader] = loadShaders(_logical_device, BASE_VERT_SHADER, BASE_FRAG_SHADER);
		try
		{
			auto pipeline_layout = createPipelineLayout(_logical_device, vert_shader, frag_shader);
			auto render_pass = createRenderPass(*_swap_chain, _logical_device);
			auto pipeline = createGraphicsPipeline(_logical_device, render_pass, pipeline_layout, vert_shader, frag_shader);
			vkDestroyShaderModule(_logical_device, vert_shader, nullptr);
			vkDestroyShaderModule(_logical_device, frag_shader, nullptr);
			_drawers.push_back(std::make_unique<Drawer>(_logical_device,
				render_pass,
				pipeline,
				pipeline_layout,
				*_swap_chain,
				_render_queue_family));
		}
		catch (const std::runtime_error&)
		{
			vkDestroyShaderModule(_logical_device, vert_shader, nullptr);
			vkDestroyShaderModule(_logical_device, frag_shader, nullptr);
			throw;
		}
	}


	Window::~Window()
	{
		vkDeviceWaitIdle(_logical_device);
		vkDestroySemaphore(_logical_device, _image_available_semaphore, nullptr);
		vkDestroySemaphore(_logical_device, _render_finished_semaphore, nullptr);
		vkDestroyFence(_logical_device, _in_flight_fence, nullptr);
		_swap_chain.reset();
		glfwDestroyWindow(_window);
	}

	void Window::initSynchronizationObjects()
	{
		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(_logical_device, &semaphore_info, nullptr, &_image_available_semaphore) != VK_SUCCESS ||
			vkCreateSemaphore(_logical_device, &semaphore_info, nullptr, &_render_finished_semaphore) != VK_SUCCESS ||
			vkCreateFence(_logical_device, &fence_info, nullptr, &_in_flight_fence) != VK_SUCCESS)
		{
			vkDestroySemaphore(_logical_device, _image_available_semaphore, nullptr);
			vkDestroySemaphore(_logical_device, _render_finished_semaphore, nullptr);
			vkDestroyFence(_logical_device, _in_flight_fence, nullptr);
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	void Window::handleEvents()
	{
		glfwPollEvents();
		_closed = glfwWindowShouldClose(_window);
	}

	void Window::present()
	{
		vkWaitForFences(_logical_device, 1, &_in_flight_fence, VK_TRUE, UINT64_MAX);
		vkResetFences(_logical_device, 1, &_in_flight_fence);

		uint32_t image_index = 0;
		vkAcquireNextImageKHR(_logical_device,
			_swap_chain->getDetails().swap_chain,
			UINT64_MAX,
			_image_available_semaphore,
			VK_NULL_HANDLE, &image_index);

		std::vector<VkCommandBuffer> command_buffers;
		for (auto& drawer : _drawers)
		{
			drawer->draw(image_index);
			auto current_command_buffers = drawer->getCommandBuffers();
			std::copy(current_command_buffers.begin(), current_command_buffers.end(),
				std::back_inserter(command_buffers));
		}


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { _image_available_semaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = command_buffers.size();
		submitInfo.pCommandBuffers = command_buffers.data();

		VkSemaphore signalSemaphores[] = { _render_finished_semaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(_render_queue, 1, &submitInfo, _in_flight_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { _swap_chain->getDetails().swap_chain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &image_index;

		vkQueuePresentKHR(_present_queue, &presentInfo);
	}

}