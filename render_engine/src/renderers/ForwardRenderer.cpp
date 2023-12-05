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
    ForwardRenderer::ForwardRenderer(Window& window,
                                     const RenderTarget& render_target,
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

        auto logical_device = window.getDevice().getLogicalDevice();
        VkRenderPass render_pass{ VK_NULL_HANDLE };
        if (vkCreateRenderPass(logical_device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS)
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

    void ForwardRenderer::addMesh(const MeshInstance* mesh_instance, int32_t priority)
    {
        const auto* mesh = mesh_instance->getMesh();
        const auto& material = mesh->getMaterial();
        const uint32_t material_id = material.getId();
        const uint32_t material_instance_id = mesh_instance->getMaterialInstance()->getId();
        const Geometry& geometry = mesh->getGeometry();

        const Shader::MetaData& vertex_shader_meta_data = material.getVertexShader().getMetaData();
        MeshBuffers mesh_buffers;
        auto& transfer_engine = getWindow().getTransferEngine();
        auto& gpu_resource_manager = getWindow().getRenderEngine().getGpuResourceManager();
        uint32_t render_queue_family_index = getWindow().getRenderEngine().getQueueFamilyIndex();
        if (geometry.positions.empty() == false)
        {
            std::vector vertex_buffer = mesh->createVertexBuffer();
            mesh_buffers.vertex_buffer = gpu_resource_manager.createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                                                    vertex_buffer.size());
            mesh_buffers.vertex_buffer->uploadUnmapped(std::span{ vertex_buffer.data(), vertex_buffer.size() },
                                                       transfer_engine, render_queue_family_index);

        }
        if (geometry.indexes.empty() == false)
        {
            mesh_buffers.index_buffer = gpu_resource_manager.createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                                                   geometry.indexes.size() * sizeof(int16_t));
            mesh_buffers.index_buffer->uploadUnmapped(std::span{ geometry.indexes.data(), geometry.indexes.size() },
                                                      transfer_engine, render_queue_family_index);
        }
        _mesh_buffers[mesh_instance->getMesh()] = std::move(mesh_buffers);
        auto& mesh_group = _meshes[priority][material_instance_id];
        mesh_group.mesh_instances.push_back(mesh_instance);
        if (mesh_group.technique == nullptr)
        {
            auto& material = mesh->getMaterial();
            _meshes[priority][material_instance_id].technique = mesh_instance->getMaterialInstance()->createTechnique(gpu_resource_manager,
                                                                                                                      getRenderPass());
        }
    }

    void ForwardRenderer::draw(uint32_t swap_chain_image_index, uint32_t frame_number)
    {
        FrameData& frame_data = getFrameData(frame_number);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        auto render_area = getRenderArea();
        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = getRenderPass();
        render_pass_info.framebuffer = getFrameBuffer(swap_chain_image_index);
        render_pass_info.renderArea = render_area;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        for (auto& mesh_group_container : _meshes | std::views::values)
        {
            for (auto& mesh_group : mesh_group_container | std::views::values)
            {
                auto push_constants_updater = mesh_group.technique->createPushConstantsUpdater(frame_data.command_buffer);

                mesh_group.technique->updateGlobalUniformBuffer(frame_number);
                mesh_group.technique->updateGlobalPushConstants(push_constants_updater);

                vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_group.technique->getPipeline());

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float)render_area.extent.width;
                viewport.height = (float)render_area.extent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(frame_data.command_buffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = { 0, 0 };
                scissor.extent = render_area.extent;
                vkCmdSetScissor(frame_data.command_buffer, 0, 1, &scissor);
                auto descriptor_sets = mesh_group.technique->collectDescriptorSets(frame_number);

                if (descriptor_sets.empty() == false)
                {
                    vkCmdBindDescriptorSets(frame_data.command_buffer,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            mesh_group.technique->getPipelineLayout(),
                                            0,
                                            descriptor_sets.size(),
                                            descriptor_sets.data(), 0, nullptr);
                }

                for (auto& mesh_instance : mesh_group.mesh_instances)
                {
                    mesh_instance->updatePushConstants(push_constants_updater);
                    auto& mesh_buffers = _mesh_buffers.at(mesh_instance->getMesh());

                    VkBuffer vertexBuffers[] = { mesh_buffers.vertex_buffer->getBuffer() };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(frame_data.command_buffer, 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(frame_data.command_buffer, mesh_buffers.index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);


                    vkCmdDrawIndexed(frame_data.command_buffer, static_cast<uint32_t>(mesh_buffers.index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);
                }
            }
        }
        vkCmdEndRenderPass(frame_data.command_buffer);

        if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void ForwardRenderer::onFrameBegin(uint32_t frame_number)
    {
        for (auto& mesh_group_map : _meshes | std::views::values)
        {
            for (auto& mesh_group : mesh_group_map | std::views::values)
            {
                for (const auto& uniform_binding : mesh_group.technique->getUniformBindings())
                {
                    if (auto texture = uniform_binding.getTextureForFrame(frame_number); texture != nullptr)
                    {
                        ResourceStateMachine::resetStages(*texture);
                    }
                    if (auto buffer = uniform_binding.getTextureForFrame(frame_number); buffer != nullptr)
                    {
                        ResourceStateMachine::resetStages(*buffer);
                    }
                }
            }
        }
    }
}
