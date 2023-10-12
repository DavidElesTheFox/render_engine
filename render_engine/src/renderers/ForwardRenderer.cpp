#include <render_engine/renderers/ForwardRenderer.h>

#include "render_engine/window/Window.h"
#include "render_engine/RenderEngine.h"
#include <ranges>
#include <volk.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Technique.h>
#include <render_engine/resources/PushConstantsUpdater.h>

namespace RenderEngine
{
	ForwardRenderer::ForwardRenderer(Window& parent,
		const SwapChain& swap_chain,
		bool last_renderer,
		std::vector<Material*> supported_materials)
		try : _window(parent)
		, _back_buffer(parent.getGpuResourceManager().getBackBufferSize())
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
			std::unique_ptr<Technique> technique = material->createTechnique(logical_device, _window.getGpuResourceManager(), _render_pass);
			_technique_map.insert(std::make_pair(material->getId(), std::move(technique)));
		}
	}
	catch (const std::exception&)
	{
		destroy();
	}

	ForwardRenderer::~ForwardRenderer()
	{
		destroy();
	}
	void ForwardRenderer::destroy()
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
		const Geometry& geometry = mesh->getGeometry();

		const Shader::MetaData& vertex_shader_meta_data = mesh->getMaterial().getVertexShader().getMetaData();
		MeshBuffers mesh_buffers;
		if (geometry.positions.empty() == false)
		{
			std::vector vertex_buffer = mesh->createVertexBuffer();
			mesh_buffers.vertex_buffer = _window.getGpuResourceManager().createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				vertex_buffer.size());
			mesh_buffers.vertex_buffer->uploadUnmapped(std::span{ vertex_buffer.data(), vertex_buffer.size() }, _window.getRenderQueue(), _command_pool);

		}
		if (geometry.indexes.empty() == false)
		{
			mesh_buffers.index_buffer = _window.getGpuResourceManager().createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				geometry.indexes.size() * sizeof(int16_t));
			mesh_buffers.index_buffer->uploadUnmapped(std::span{ geometry.indexes.data(), geometry.indexes.size() }, _window.getRenderQueue(), _command_pool);
		}
		_meshes[priority][mesh->getMaterial().getId()].mesh_buffers[mesh] = std::move(mesh_buffers);
		_meshes[priority][mesh->getMaterial().getId()].technique = _technique_map[mesh->getMaterial().getId()].get();
	}

	void ForwardRenderer::draw(uint32_t swap_chain_image_index, uint32_t frame_number)
	{
		FrameData& frame_data = _back_buffer[frame_number];


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

				auto push_constants_updater = mesh_group.technique->createPushConstantsUpdater(frame_data.command_buffer);
				
				for (auto& [mesh, mesh_buffers] : mesh_group.mesh_buffers)
				{
					mesh_group.technique->updateConstants(mesh, push_constants_updater);

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
