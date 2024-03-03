#include <render_engine/renderers/ForwardRenderer.h>

#include <volk.h>


#include "render_engine/Device.h"
#include "render_engine/window/Window.h"
#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/PushConstantsUpdater.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/resources/Technique.h>

#include <ranges>

namespace RenderEngine
{
    ForwardRenderer::ForwardRenderer(IWindow& window,
                                     RenderTarget render_target,
                                     bool last_renderer)
        try : SingleColorOutputRenderer(window)
    {

        VkAttachmentDescription color_attachment{};
        color_attachment.format = render_target.getImageFormat();
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = last_renderer ? render_target.getLayout() : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &color_attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;

        auto& logical_device = window.getDevice().getLogicalDevice();
        VkRenderPass render_pass{ VK_NULL_HANDLE };
        if (logical_device->vkCreateRenderPass(*logical_device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
        initializeRendererOutput(render_target, render_pass, window.getRenderEngine().getGpuResourceManager().getBackBufferSize());
    }
    catch (const std::exception&)
    {
        destroyRenderOutput();
    }

    ForwardRenderer::~ForwardRenderer()
    {}

    void ForwardRenderer::addMesh(const MeshInstance* mesh_instance)
    {
        const auto* mesh = mesh_instance->getMesh();
        const uint32_t material_instance_id = mesh_instance->getMaterialInstance()->getId();
        auto& gpu_resource_manager = getWindow().getRenderEngine().getGpuResourceManager();

        if (_mesh_buffers.contains(mesh) == false)
        {
            const Geometry& geometry = mesh->getGeometry();

            MeshBuffers mesh_buffers;
            if (geometry.positions.empty() == false)
            {
                std::vector vertex_buffer = mesh->createVertexBuffer();
                mesh_buffers.vertex_buffer = gpu_resource_manager.createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                                                        vertex_buffer.size());
                getWindow().getDevice().getDataTransferContext().getScheduler().upload(mesh_buffers.vertex_buffer.get(),
                                                                                       std::span(vertex_buffer),
                                                                                       getWindow().getRenderEngine().getTransferCommandContext(),
                                                                                       mesh_buffers.vertex_buffer->getResourceState().clone());

            }
            if (geometry.indexes.empty() == false)
            {
                mesh_buffers.index_buffer = gpu_resource_manager.createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                                                       geometry.indexes.size() * sizeof(int16_t));
                getWindow().getDevice().getDataTransferContext().getScheduler().upload(mesh_buffers.index_buffer.get(),
                                                                                       std::span(geometry.indexes),
                                                                                       getWindow().getRenderEngine().getTransferCommandContext(),
                                                                                       mesh_buffers.index_buffer->getResourceState().clone());
            }
            _mesh_buffers[mesh] = std::move(mesh_buffers);
        }
        auto it = std::ranges::find_if(_meshes,
                                       [&](const auto& mesh_group) { return mesh_group.technique->getMaterialInstance().getId() == material_instance_id; });
        if (it != _meshes.end())
        {
            it->mesh_instances.push_back(mesh_instance);

        }
        else
        {
            MeshGroup mesh_group;
            mesh_group.technique = mesh_instance->getMaterialInstance()->createTechnique(gpu_resource_manager,
                                                                                         {},
                                                                                         getRenderPass(),
                                                                                         0);
            mesh_group.mesh_instances.push_back(mesh_instance);
            _meshes.push_back(std::move(mesh_group));
        }
    }

    void ForwardRenderer::draw(uint32_t swap_chain_image_index)
    {
        FrameData& frame_data = getFrameData(swap_chain_image_index);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (getLogicalDevice()->vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        auto renderer_marker = _performance_markers.createMarker(frame_data.command_buffer,
                                                                 "ForwardRenderer");
        auto render_area = getRenderArea();
        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = getRenderPass();
        render_pass_info.framebuffer = getFrameBuffer(swap_chain_image_index);
        render_pass_info.renderArea = render_area;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        getLogicalDevice()->vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        for (auto& mesh_group : _meshes)
        {
            auto technique_marker = _performance_markers.createMarker(frame_data.command_buffer,
                                                                      mesh_group.technique->getMaterialInstance().getMaterial().getName());

            MaterialInstance::UpdateContext material_update_context = mesh_group.technique->onFrameBegin(swap_chain_image_index, frame_data.command_buffer);

            getLogicalDevice()->vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_group.technique->getPipeline());

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)render_area.extent.width;
            viewport.height = (float)render_area.extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            getLogicalDevice()->vkCmdSetViewport(frame_data.command_buffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = render_area.extent;
            getLogicalDevice()->vkCmdSetScissor(frame_data.command_buffer, 0, 1, &scissor);
            auto descriptor_sets = mesh_group.technique->collectDescriptorSets(swap_chain_image_index);

            if (descriptor_sets.empty() == false)
            {
                getLogicalDevice()->vkCmdBindDescriptorSets(frame_data.command_buffer,
                                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                            mesh_group.technique->getPipelineLayout(),
                                                            0,
                                                            static_cast<uint32_t>(descriptor_sets.size()),
                                                            descriptor_sets.data(),
                                                            0,
                                                            nullptr);
            }

            for (auto& mesh_instance : mesh_group.mesh_instances)
            {
                mesh_group.technique->onDraw(material_update_context, mesh_instance);
                auto& mesh_buffers = _mesh_buffers.at(mesh_instance->getMesh());

                VkBuffer vertexBuffers[] = { mesh_buffers.vertex_buffer->getBuffer() };
                VkDeviceSize offsets[] = { 0 };
                getLogicalDevice()->vkCmdBindVertexBuffers(frame_data.command_buffer, 0, 1, vertexBuffers, offsets);
                getLogicalDevice()->vkCmdBindIndexBuffer(frame_data.command_buffer, mesh_buffers.index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);


                getLogicalDevice()->vkCmdDrawIndexed(frame_data.command_buffer, static_cast<uint32_t>(mesh_buffers.index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);
            }
            technique_marker.finish();
        }
        getLogicalDevice()->vkCmdEndRenderPass(frame_data.command_buffer);

        if (getLogicalDevice()->vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void ForwardRenderer::onFrameBegin(uint32_t frame_number)
    {
        for (auto& mesh_group : _meshes)
        {
            for (const auto& uniform_binding : mesh_group.technique->getUniformBindings())
            {
                if (auto texture = uniform_binding->getTextureForFrame(frame_number); texture != nullptr)
                {
                    ResourceStateMachine::resetStages(*texture);
                }
                if (auto buffer = uniform_binding->getTextureForFrame(frame_number); buffer != nullptr)
                {
                    ResourceStateMachine::resetStages(*buffer);
                }
            }
        }
    }
}
