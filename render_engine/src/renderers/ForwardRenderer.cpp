#include <render_engine/renderers/ForwardRenderer.h>

#include <volk.h>


#include "render_engine/Device.h"
#include "render_engine/window/Window.h"
#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Material.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/PushConstantsUpdater.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/resources/Technique.h>

#include <render_engine/FrameBuffer.h>
#include <render_engine/RenderPass.h>

#include <ranges>

namespace RenderEngine
{
    ForwardRenderer::ForwardRenderer(RefObj<IRenderEngine> render_engine,
                                     RenderTarget render_target,
                                     bool use_internal_command_buffers)
        : _render_engine(render_engine)
        , _render_target(std::move(render_target))
    {

        RenderPassBuilder render_pass_builder;

        {
            VkAttachmentDescription color_attachment{};
            color_attachment.format = _render_target.getImageFormat();
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = _render_target.getLoadOperation();
            color_attachment.storeOp = _render_target.getStoreOperation();
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = _render_target.getInitialLayout();
            color_attachment.finalLayout = _render_target.getFinalLayout();
            render_pass_builder.addAttachment(std::move(color_attachment));
        }

        {
            RenderPassBuilder::SubPass main_pass("main_pass", VK_PIPELINE_BIND_POINT_GRAPHICS);
            main_pass.addColorAttachment({ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
            render_pass_builder.addSubPass(std::move(main_pass));
        }
        auto& logical_device = getLogicalDevice();

        _render_pass = std::make_unique<RenderPass>(render_pass_builder.build(logical_device));
        for (uint32_t i = 0; i < render_engine->getBackBufferSize(); ++i)
        {
            auto frame_buffer_builder = _render_pass->createFrameBufferBuilder();
            frame_buffer_builder.setAttachment(0, _render_target.getTextureView(i));
            _frame_buffers.emplace_back(frame_buffer_builder.build(_render_target.getWidth(), _render_target.getHeight(), logical_device));
        }

        if (use_internal_command_buffers)
        {
            for (uint32_t i = 0; i < render_engine->getBackBufferSize(); ++i)
            {
                VkCommandBuffer command_buffer = _render_engine->getCommandBufferContext().getFactory()->createCommandBuffer(i, 0);

                _internal_command_buffers.push_back(command_buffer);
            }
        }
    }


    ForwardRenderer::~ForwardRenderer()
    {}

    void ForwardRenderer::addMesh(const MeshInstance* mesh_instance)
    {
        PROFILE_SCOPE();
        const auto* mesh = mesh_instance->getMesh();
        const uint32_t material_instance_id = mesh_instance->getMaterialInstance()->getId();
        auto& gpu_resource_manager = _render_engine->getGpuResourceManager();

        if (_mesh_buffers.contains(mesh) == false)
        {
            const Geometry& geometry = mesh->getGeometry();

            MeshBuffers mesh_buffers;
            if (geometry.positions.empty() == false)
            {
                std::vector vertex_buffer = mesh->createVertexBuffer();
                mesh_buffers.vertex_buffer = gpu_resource_manager.createAttributeBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                                                        vertex_buffer.size());
                _render_engine->getDevice().getDataTransferContext().getScheduler().upload(mesh_buffers.vertex_buffer.get(),
                                                                                           std::span(vertex_buffer),
                                                                                           _render_engine->getTransferEngine().getCommandBufferFactory(),
                                                                                           mesh_buffers.vertex_buffer->getResourceState(SubmitScope{}).clone());

            }
            if (geometry.indexes.empty() == false)
            {
                mesh_buffers.index_buffer = gpu_resource_manager.createAttributeBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                                                       geometry.indexes.size() * sizeof(int16_t));
                _render_engine->getDevice().getDataTransferContext().getScheduler().upload(mesh_buffers.index_buffer.get(),
                                                                                           std::span(geometry.indexes),
                                                                                           _render_engine->getTransferEngine().getCommandBufferFactory(),
                                                                                           mesh_buffers.index_buffer->getResourceState(SubmitScope{}).clone());
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
                                                                                         _render_pass->get(),
                                                                                         0);
            mesh_group.mesh_instances.push_back(mesh_instance);
            _meshes.push_back(std::move(mesh_group));
        }
    }
    void ForwardRenderer::draw(VkCommandBuffer command_buffer, uint32_t swap_chain_image_index)
    {
        PROFILE_SCOPE();

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (getLogicalDevice()->vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        auto renderer_marker = _performance_markers.createMarker(command_buffer,
                                                                 "ForwardRenderer");

        {
            auto render_area = getRenderArea();
            VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            auto render_pass_scope = _render_pass->begin(getLogicalDevice(),
                                                         command_buffer,
                                                         render_area,
                                                         _frame_buffers[swap_chain_image_index],
                                                         { clear_color });
            for (auto& mesh_group : _meshes)
            {
                auto technique_marker = _performance_markers.createMarker(command_buffer,
                                                                          mesh_group.technique->getMaterialInstance().getMaterial().getName());

                MaterialInstance::UpdateContext material_update_context = mesh_group.technique->onFrameBegin(swap_chain_image_index, command_buffer);

                getLogicalDevice()->vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_group.technique->getPipeline());

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float)render_area.extent.width;
                viewport.height = (float)render_area.extent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                getLogicalDevice()->vkCmdSetViewport(command_buffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = { 0, 0 };
                scissor.extent = render_area.extent;
                getLogicalDevice()->vkCmdSetScissor(command_buffer, 0, 1, &scissor);
                auto descriptor_sets = mesh_group.technique->collectDescriptorSets(swap_chain_image_index);

                if (descriptor_sets.empty() == false)
                {
                    getLogicalDevice()->vkCmdBindDescriptorSets(command_buffer,
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
                    getLogicalDevice()->vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
                    getLogicalDevice()->vkCmdBindIndexBuffer(command_buffer, mesh_buffers.index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);


                    getLogicalDevice()->vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh_buffers.index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);
                }
                technique_marker.finish();
            }
        }
        if (getLogicalDevice()->vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void ForwardRenderer::draw(uint32_t swap_chain_image_index)
    {
        draw(_internal_command_buffers[swap_chain_image_index], swap_chain_image_index);
    }

    void ForwardRenderer::onFrameBegin(uint32_t frame_number)
    {
        PROFILE_SCOPE();
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
