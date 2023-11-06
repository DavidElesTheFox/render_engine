#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/window/Window.h>
#include <render_engine/Device.h>

#include <stdexcept>
#include <fstream>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

namespace
{
	using namespace RenderEngine;

	VkRenderPass createRenderPass(const SwapChain& swap_chain, VkDevice logical_device, bool first_renderer) {
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swap_chain.getDetails().image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = first_renderer ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = first_renderer ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

	VkDescriptorPool createDescriptorPool(VkDevice logical_device) {
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool result;
		if (vkCreateDescriptorPool(logical_device, &pool_info, nullptr, &result) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
		return result;
	}


}

namespace RenderEngine
{
	std::set<ImGuiContext*> UIRenderer::kInvalidContexts;

	UIRenderer::UIRenderer(Window& window,
		const SwapChain& swap_chain,
		uint32_t back_buffer_size,
		bool first_renderer)
		: _window(window)
	{
		_imgui_context_during_init = ImGui::GetCurrentContext();
		_imgui_context = ImGui::CreateContext();
		ImGui::SetCurrentContext(_imgui_context);

		ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
			return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
			}, &window.getDevice().getVulkanInstance());

		ImGui_ImplGlfw_InitForVulkan(_window.getWindowHandle(), true);


		auto logical_device = window.getDevice().getLogicalDevice();

		try
		{
			_descriptor_pool = createDescriptorPool(logical_device);
			_render_pass = createRenderPass(swap_chain, logical_device, first_renderer);

			_back_buffer.resize(back_buffer_size);
			_render_area.offset = { 0, 0 };
			_render_area.extent = swap_chain.getDetails().extent;
			createFrameBuffers(swap_chain);
			createCommandPool(window.getRenderEngine().getRenderQueueFamily());
			createCommandBuffer();

			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = _window.getDevice().getVulkanInstance();
			init_info.PhysicalDevice = _window.getDevice().getPhysicalDevice();
			init_info.Device = logical_device;
			init_info.Queue = window.getRenderEngine().getRenderQueue();
			init_info.DescriptorPool = _descriptor_pool;
			init_info.MinImageCount = back_buffer_size;
			init_info.ImageCount = back_buffer_size;
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

			ImGui_ImplVulkan_Init(&init_info, _render_pass);
			{
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = _command_pool;
				allocInfo.commandBufferCount = 1;

				VkCommandBuffer command_buffer;
				vkAllocateCommandBuffers(logical_device, &allocInfo, &command_buffer);
				
				VkCommandBufferSubmitInfo command_buffer_info{};
				command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
				command_buffer_info.commandBuffer = command_buffer;
				command_buffer_info.pNext = nullptr;

				VkCommandBufferBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(command_buffer, &begin_info);

				ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

				vkEndCommandBuffer(command_buffer);

				VkSubmitInfo2 submit_info{}; 
				submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
				submit_info.commandBufferInfoCount = 1;
				submit_info.pCommandBufferInfos = &command_buffer_info;
				vkQueueSubmit2(_window.getRenderEngine().getRenderQueue(), 1, &submit_info, VK_NULL_HANDLE);
				vkQueueWaitIdle(_window.getRenderEngine().getRenderQueue());

				vkFreeCommandBuffers(logical_device, _command_pool, 1, &command_buffer);
			}
			ImGui_ImplVulkan_DestroyFontUploadObjects();

		}
		catch (const std::runtime_error&)
		{
			auto logical_device = window.getDevice().getLogicalDevice();
			vkDestroyCommandPool(logical_device, _command_pool, nullptr);

			for (auto framebuffer : _frame_buffers) {
				vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
			}
			vkDestroyRenderPass(logical_device, _render_pass, nullptr);
			throw;
		}
	}

	UIRenderer::~UIRenderer()
	{
		ImGui::SetCurrentContext(_imgui_context);
		auto logical_device = _window.getDevice().getLogicalDevice();
		vkDestroyCommandPool(logical_device, _command_pool, nullptr);

		resetFrameBuffers();

		vkDestroyDescriptorPool(logical_device, _descriptor_pool, nullptr);

		vkDestroyRenderPass(logical_device, _render_pass, nullptr);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		registerDeletedContext(_imgui_context);
		ImGui::DestroyContext(_imgui_context);
		if (isValidContext(_imgui_context_during_init))
		{
			ImGui::SetCurrentContext(_imgui_context_during_init);
		}

	}
	void UIRenderer::resetFrameBuffers()
	{
		auto logical_device = _window.getDevice().getLogicalDevice();

		for (auto framebuffer : _frame_buffers) {
			vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
		}
	}
	void UIRenderer::beforeReinit()
	{
		resetFrameBuffers();
	}
	void UIRenderer::finalizeReinit(const SwapChain& swap_chain)
	{
		createFrameBuffers(swap_chain);
		_render_area.offset = { 0, 0 };
		_render_area.extent = swap_chain.getDetails().extent;
	}
	void UIRenderer::draw(const VkFramebuffer& frame_buffer, uint32_t frame_number)
	{
		if (_on_gui == nullptr)
		{
			return;
		}
		int focused = glfwGetWindowAttrib(_window.getWindowHandle(), GLFW_FOCUSED);


		if (focused)
		{
			if (ImGui::GetCurrentContext() != _imgui_context)
			{
				ImGui::SetCurrentContext(_imgui_context);
			}
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
		FrameData& frame_data = getFrameData(frame_number);

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
		if (focused)
		{
			_on_gui();
			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame_data.command_buffer);
		}
		vkCmdEndRenderPass(frame_data.command_buffer);

		if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void UIRenderer::registerDeletedContext(ImGuiContext* context)
	{
		kInvalidContexts.insert(context);
	}

	bool UIRenderer::isValidContext(ImGuiContext* context)
	{
		return kInvalidContexts.contains(context) == false;
	}

	void UIRenderer::createFrameBuffers(const SwapChain& swap_chain)
	{
		auto logical_device = _window.getDevice().getLogicalDevice();

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
	bool UIRenderer::createFrameBuffer(const SwapChain& swap_chain, uint32_t frame_buffer_index)
	{
		auto logical_device = _window.getDevice().getLogicalDevice();

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
	void UIRenderer::createCommandPool(uint32_t render_queue_family)
	{
		auto logical_device = _window.getDevice().getLogicalDevice();

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = render_queue_family;
		if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	void UIRenderer::createCommandBuffer()
	{
		auto logical_device = _window.getDevice().getLogicalDevice();

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