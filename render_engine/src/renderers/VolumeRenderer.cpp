#include <render_engine/renderers/VolumeRenderer.h>

#include <render_engine/RenderContext.h>
#include <render_engine/resources/Technique.h>

#include <render_engine/cuda_compute/DistanceFieldTask.h>

namespace RenderEngine
{
    namespace
    {

        class DistanceFieldFinishedCallback : public CudaCompute::IComputeCallback
        {
        public:
            static constexpr auto kSemaphoreName = "distance_field_ready";
            static constexpr auto kReadyValue = 2;
            static constexpr auto kProcessValue = 1;
            explicit DistanceFieldFinishedCallback(SyncObject& sync_object)
                : _sync_object(sync_object)
            {}

            void call() final
            {
                _sync_object.signalSemaphore(kSemaphoreName, kReadyValue);
                _is_called = true;
            }
            bool isCalled() const final
            {
                return _is_called;
            }
        private:
            SyncObject& _sync_object;
            std::atomic_bool _is_called{ false };
        };
        /*
            #version 450

            layout(location = 0) in vec3 inTexCoord;

            layout(location = 0) out vec4 outTexCoord;

            void main() {
                outTexCoord = vec4(inTexCoord, 0.0);
            }
        */
        const std::vector<uint32_t> pre_pass_fragment_shader_code =
        {
            0x07230203,0x00010000,0x000d000b,0x00000016,
            0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,
            0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0007000f,0x00000004,0x00000004,0x6e69616d,
            0x00000000,0x00000009,0x0000000c,0x00030010,
            0x00000004,0x00000007,0x00030003,0x00000002,
            0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,
            0x70635f45,0x74735f70,0x5f656c79,0x656e696c,
            0x7269645f,0x69746365,0x00006576,0x00080004,
            0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,
            0x69645f65,0x74636572,0x00657669,0x00040005,
            0x00000004,0x6e69616d,0x00000000,0x00050005,
            0x00000009,0x5474756f,0x6f437865,0x0064726f,
            0x00050005,0x0000000c,0x65546e69,0x6f6f4378,
            0x00006472,0x00050005,0x00000013,0x6f6c6f43,
            0x66664f72,0x00746573,0x00040006,0x00000013,
            0x00000000,0x00000072,0x00050005,0x00000015,
            0x6f6c6f63,0x66664f72,0x00746573,0x00040047,
            0x00000009,0x0000001e,0x00000000,0x00040047,
            0x0000000c,0x0000001e,0x00000000,0x00050048,
            0x00000013,0x00000000,0x00000023,0x00000000,
            0x00030047,0x00000013,0x00000002,0x00040047,
            0x00000015,0x00000022,0x00000000,0x00040047,
            0x00000015,0x00000021,0x00000000,0x00020013,
            0x00000002,0x00030021,0x00000003,0x00000002,
            0x00030016,0x00000006,0x00000020,0x00040017,
            0x00000007,0x00000006,0x00000004,0x00040020,
            0x00000008,0x00000003,0x00000007,0x0004003b,
            0x00000008,0x00000009,0x00000003,0x00040017,
            0x0000000a,0x00000006,0x00000003,0x00040020,
            0x0000000b,0x00000001,0x0000000a,0x0004003b,
            0x0000000b,0x0000000c,0x00000001,0x0004002b,
            0x00000006,0x0000000e,0x00000000,0x0003001e,
            0x00000013,0x00000006,0x00040020,0x00000014,
            0x00000002,0x00000013,0x0004003b,0x00000014,
            0x00000015,0x00000002,0x00050036,0x00000002,
            0x00000004,0x00000000,0x00000003,0x000200f8,
            0x00000005,0x0004003d,0x0000000a,0x0000000d,
            0x0000000c,0x00050051,0x00000006,0x0000000f,
            0x0000000d,0x00000000,0x00050051,0x00000006,
            0x00000010,0x0000000d,0x00000001,0x00050051,
            0x00000006,0x00000011,0x0000000d,0x00000002,
            0x00070050,0x00000007,0x00000012,0x0000000f,
            0x00000010,0x00000011,0x0000000e,0x0003003e,
            0x00000009,0x00000012,0x000100fd,0x00010038
        };

        std::unique_ptr<Shader> createPrePassFragmentShader()
        {
            return std::make_unique<Shader>(pre_pass_fragment_shader_code, Shader::MetaData{});
        }
    }

    VolumeRenderer::VolumeRenderer(IWindow& window, RenderTarget render_target, bool last_renderer)
        try : SingleColorOutputRenderer(window)
        , _render_target(render_target)

    {
        std::array<VkAttachmentDescription, 3> attachments = {
            VkAttachmentDescription{},
            VkAttachmentDescription{},
            VkAttachmentDescription{}
        };
        // Final attachment
        {
            VkAttachmentDescription& color_attachment = attachments[0];
            color_attachment.format = render_target.getImageFormat();
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = last_renderer ? render_target.getLayout() : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        {
            VkAttachmentDescription& front_face_attachment = attachments[1];
            front_face_attachment.format = render_target.getImageFormat();
            front_face_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            front_face_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            front_face_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            front_face_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            front_face_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            front_face_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            front_face_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        {
            VkAttachmentDescription& back_face_attachment = attachments[2];
            back_face_attachment.format = render_target.getImageFormat();
            back_face_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            back_face_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            back_face_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            back_face_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            back_face_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            back_face_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            back_face_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        std::array<VkSubpassDescription, 3> subpass_descriptions = {
            VkSubpassDescription{},
            VkSubpassDescription{},
            VkSubpassDescription{}
        };
        // Front face
        VkAttachmentReference front_face_reference = { .attachment = 1, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        subpass_descriptions[0].colorAttachmentCount = 1;
        subpass_descriptions[0].pColorAttachments = &front_face_reference;
        subpass_descriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Back face
        VkAttachmentReference back_face_reference = { .attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        subpass_descriptions[1].colorAttachmentCount = 1;
        subpass_descriptions[1].pColorAttachments = &back_face_reference;
        subpass_descriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Volume rendering
        VkAttachmentReference color_reference = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        std::array<VkAttachmentReference, 2> input_references =
        {
            VkAttachmentReference{.attachment = 1, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            VkAttachmentReference{.attachment = 2, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
        };

        subpass_descriptions[2].colorAttachmentCount = 1;
        subpass_descriptions[2].pColorAttachments = &color_reference;
        subpass_descriptions[2].inputAttachmentCount = static_cast<uint32_t>(input_references.size());
        subpass_descriptions[2].pInputAttachments = input_references.data();
        subpass_descriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        std::array<VkSubpassDependency, 3> dependencies;
        // This dependency transitions the input attachment from color attachment to shader read
        dependencies[0] = VkSubpassDependency{};
        dependencies[0].srcSubpass = 0;
        dependencies[0].dstSubpass = 2;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1] = VkSubpassDependency{};
        dependencies[1].srcSubpass = 1;
        dependencies[1].dstSubpass = 2;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[2] = VkSubpassDependency{};
        dependencies[2].srcSubpass = 0;
        dependencies[2].dstSubpass = 1;
        dependencies[2].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[2].srcAccessMask = VK_ACCESS_NONE;
        dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[2].dstAccessMask = VK_ACCESS_NONE;
        dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpass_descriptions.size());
        renderPassInfo.pSubpasses = subpass_descriptions.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        auto& logical_device = window.getDevice().getLogicalDevice();
        VkRenderPass render_pass{ VK_NULL_HANDLE };
        if (logical_device->vkCreateRenderPass(*logical_device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }

        const uint32_t back_buffer_size = window.getRenderEngine().getGpuResourceManager().getBackBufferSize();

        auto render_pass_attachments = createFrameBuffersAndAttachments(render_target);

        initializeRendererOutput(render_target,
                                 render_pass,
                                 back_buffer_size,
                                 render_pass_attachments);


    }
    catch (const std::exception&)
    {
        destroyRenderOutput();
    }

    void VolumeRenderer::onFrameBegin(uint32_t image_index)
    {
        for (MeshGroup& mesh_group : _meshes)
        {
            resetResourceStatesOf(*mesh_group.technique_data.front_face_technique, image_index);
            resetResourceStatesOf(*mesh_group.technique_data.back_face_technique, image_index);
            resetResourceStatesOf(*mesh_group.technique_data.volume_technique, image_index);
        }
    }

    void VolumeRenderer::addVolumeObject(const VolumetricObjectInstance* mesh_instance)
    {
        const auto* mesh = mesh_instance->getMesh();
        const uint32_t material_instance_id = mesh_instance->getMaterialInstance()->getId();
        const Geometry& geometry = mesh->getGeometry();


        MeshBuffers mesh_buffers;
        auto& gpu_resource_manager = getWindow().getRenderEngine().getGpuResourceManager();
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
        _mesh_buffers[mesh_instance->getMesh()] = std::move(mesh_buffers);
        if (mesh_instance->getVolumeMaterialInstance()->getVolumeMaterial().isRequireDistanceField())
        {
            auto it = std::ranges::find_if(_meshes_with_distance_field,
                                           [&](const MeshGroup& mesh_group)
                                           {
                                               return mesh_group.technique_data.volume_technique->getMaterialInstance().getId()
                                                   == material_instance_id;
                                           });
            if (it == _meshes_with_distance_field.end())
            {
                _meshes_with_distance_field.push_back(MeshGroup{ .technique_data = createTechniqueDataFor(*mesh_instance), .meshes = {mesh_instance} });
            }
            else
            {
                it->meshes.push_back(mesh_instance);
            }
        }
        else
        {
            auto it = std::ranges::find_if(_meshes,
                                           [&](const MeshGroup& mesh_group)
                                           {
                                               return mesh_group.technique_data.volume_technique->getMaterialInstance().getId()
                                                   == material_instance_id;
                                           });
            if (it == _meshes.end())
            {
                _meshes.push_back(MeshGroup{ .technique_data = createTechniqueDataFor(*mesh_instance), .meshes = {mesh_instance} });
            }
            else
            {
                it->meshes.push_back(mesh_instance);
            }
        }
    }

    SyncOperations VolumeRenderer::getSyncOperations(uint32_t image_index)
    {
        SyncOperations result;
        for (auto& mesh_group : _meshes_with_distance_field)
        {
            result = result.createUnionWith(mesh_group.technique_data.synchronization_objects[image_index].getOperationsGroup(SyncGroups::kExternal));
        }
        return result;
    }

    void VolumeRenderer::draw(uint32_t swap_chain_image_index)
    {
        FrameData& frame_data = getFrameData(swap_chain_image_index);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (getLogicalDevice()->vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        auto render_area = getRenderArea();
        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = getRenderPass();
        render_pass_info.framebuffer = getFrameBuffer(swap_chain_image_index);
        render_pass_info.renderArea = render_area;

        std::array<VkClearValue, 3> clearColors = {
            VkClearValue{{0.0f, 0.0f, 0.0f, 1.0f}},
            VkClearValue{{0.0f, 0.0f, 0.0f, 1.0f}},
            VkClearValue{{0.0f, 0.0f, 0.0f, 1.0f}}
        };
        render_pass_info.clearValueCount = static_cast<uint32_t>(clearColors.size());
        render_pass_info.pClearValues = clearColors.data();
        for (auto& mesh_group : _meshes_with_distance_field)
        {
            ResourceStateMachine resource_state_machine{ getLogicalDevice() };
            resource_state_machine.recordStateChange(mesh_group.technique_data.distance_field_textures[swap_chain_image_index].get(),
                                                     mesh_group.technique_data.distance_field_textures[swap_chain_image_index]->getResourceState().clone()
                                                     .setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                                     .setPipelineStage(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
                                                     .setAccessFlag(VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));
            resource_state_machine.commitChanges(frame_data.command_buffer);
        }
        getLogicalDevice()->vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        for (auto& mesh_group : _meshes)
        {
            constexpr bool calculate_distance_field = false;
            renderMeshGroup(mesh_group, swap_chain_image_index, frame_data, calculate_distance_field);
        }
        for (auto& mesh_group : _meshes_with_distance_field)
        {
            constexpr bool calculate_distance_field = true;
            renderMeshGroup(mesh_group, swap_chain_image_index, frame_data, calculate_distance_field);
        }


        getLogicalDevice()->vkCmdEndRenderPass(frame_data.command_buffer);

        if (getLogicalDevice()->vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }


    }
    void VolumeRenderer::renderMeshGroup(MeshGroup& mesh_group, uint32_t swap_chain_image_index, FrameData& frame_data, bool calculate_distance_field)
    {
        if (calculate_distance_field)
        {
            startDistanceFieldTask(mesh_group, swap_chain_image_index);
            cleanupDistanceFieldTasks(mesh_group);
        }
        drawWithTechnique("VolumeRenderer - front_face",
                          *mesh_group.technique_data.front_face_technique,
                          mesh_group.meshes,
                          frame_data,
                          swap_chain_image_index);
        getLogicalDevice()->vkCmdNextSubpass(frame_data.command_buffer, VK_SUBPASS_CONTENTS_INLINE);

        drawWithTechnique("VolumeRenderer - back_face",
                          *mesh_group.technique_data.back_face_technique,
                          mesh_group.meshes,
                          frame_data,
                          swap_chain_image_index);
        getLogicalDevice()->vkCmdNextSubpass(frame_data.command_buffer, VK_SUBPASS_CONTENTS_INLINE);

        drawWithTechnique("VolumeRenderer - volume_render",
                          *mesh_group.technique_data.volume_technique,
                          mesh_group.meshes,
                          frame_data,
                          swap_chain_image_index);

    }

    void VolumeRenderer::startDistanceFieldTask(MeshGroup& mesh_group, uint32_t swap_chain_image_index)
    {
        auto& synchronization_object = mesh_group.technique_data.synchronization_objects[swap_chain_image_index];

        synchronization_object.waitSemaphore(DistanceFieldFinishedCallback::kSemaphoreName,
                                             DistanceFieldFinishedCallback::kReadyValue);

        synchronization_object.stepTimeline(DistanceFieldFinishedCallback::kSemaphoreName);

        const auto* input_surface = mesh_group.technique_data.intensity_surface[swap_chain_image_index].get();
        const auto& intensity_image = mesh_group.technique_data.distance_field_textures[swap_chain_image_index]->getImage();
        CudaCompute::DistanceFieldTask::Description task_description{};
        task_description.width = intensity_image.getWidth();
        task_description.height = intensity_image.getHeight();
        task_description.depth = intensity_image.getDepth();
        task_description.input_data = input_surface->getSurface();
        task_description.output_data = mesh_group.technique_data.distance_field_surface[swap_chain_image_index]->getSurface();
        task_description.on_finished_callback = std::make_unique<DistanceFieldFinishedCallback>(mesh_group.technique_data.synchronization_objects[swap_chain_image_index]);

        task_description.segmentation_threshold = mesh_group.technique_data.segmentation_threshold;

        CudaCompute::DistanceFieldTask distance_field_task(CudaCompute::DistanceFieldTask::ExecutionParameters{ .thread_count_per_block = 512 });

        assert(getWindow().getDevice().hasCudaDevice());

        auto& cuda_device = getWindow().getDevice().getCudaDevice();
        distance_field_task.execute(std::move(task_description),
                                    &cuda_device);
        mesh_group.tasks.emplace_back(std::move(distance_field_task));
    }

    void VolumeRenderer::cleanupDistanceFieldTasks(MeshGroup& mesh_group)
    {
        std::erase_if(mesh_group.tasks, [](auto& task) { return task.isReady(); });
    }

    void VolumeRenderer::drawWithTechnique(const std::string& subpass_name,
                                           Technique& technique,
                                           const std::vector<const VolumetricObjectInstance*>& meshes,
                                           FrameData& frame_data,
                                           uint32_t swap_chain_image_index)
    {
        auto marker = _performance_markers.createMarker(frame_data.command_buffer, subpass_name);


        MaterialInstance::UpdateContext material_update_context = technique.onFrameBegin(swap_chain_image_index, frame_data.command_buffer);
        getLogicalDevice()->vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique.getPipeline());
        auto render_area = getRenderArea();

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
        auto descriptor_sets = technique.collectDescriptorSets(swap_chain_image_index);

        if (descriptor_sets.empty() == false)
        {
            getLogicalDevice()->vkCmdBindDescriptorSets(frame_data.command_buffer,
                                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                        technique.getPipelineLayout(),
                                                        0,
                                                        static_cast<uint32_t>(descriptor_sets.size()),
                                                        descriptor_sets.data(),
                                                        0,
                                                        nullptr);
        }

        for (auto& mesh_instance : meshes)
        {
            technique.onDraw(material_update_context, mesh_instance);
            auto& mesh_buffers = _mesh_buffers.at(mesh_instance->getMesh());
            // TODO: These draw calls can be done with only one binding. Same for Forward Renderer
            VkBuffer vertexBuffers[] = { mesh_buffers.vertex_buffer->getBuffer() };
            VkDeviceSize offsets[] = { 0 };
            getLogicalDevice()->vkCmdBindVertexBuffers(frame_data.command_buffer, 0, 1, vertexBuffers, offsets);
            getLogicalDevice()->vkCmdBindIndexBuffer(frame_data.command_buffer, mesh_buffers.index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

            getLogicalDevice()->vkCmdDrawIndexed(frame_data.command_buffer, static_cast<uint32_t>(mesh_buffers.index_buffer->getDeviceSize() / sizeof(uint16_t)), 1, 0, 0, 0);
        }
        marker.finish();
    }

    void VolumeRenderer::initializeFrameBuffers(uint32_t back_buffer_count, const Image& ethalon_image)
    {
        initializeFrameBufferData(back_buffer_count, ethalon_image, &_front_face_frame_buffer);
        initializeFrameBufferData(back_buffer_count, ethalon_image, &_back_face_frame_buffer);
    }
    void VolumeRenderer::initializeFrameBufferData(uint32_t back_buffer_count, const Image& ethalon_image, FrameBufferData* frame_buffer_data)
    {
        frame_buffer_data->textures_per_back_buffer.clear();
        frame_buffer_data->texture_views_per_back_buffer.clear();

        Texture::SamplerData sampler_data{};
        for (uint32_t i = 0; i < back_buffer_count; ++i)
        {
            frame_buffer_data->textures_per_back_buffer.push_back(getTextureFactory().createNoUpload(ethalon_image,
                                                                                                     VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                                     VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT));
            auto& texture = frame_buffer_data->textures_per_back_buffer.back();
            auto texture_view = texture->createTextureView(Texture::ImageViewData{}, sampler_data);

            frame_buffer_data->texture_views_per_back_buffer.push_back(std::move(texture_view));
        }
    }

    VolumeRenderer::TechniqueData VolumeRenderer::createTechniqueDataFor(const VolumetricObjectInstance& mesh)
    {
        TechniqueData result;


        auto& gpu_resource_manager = getWindow().getRenderEngine().getGpuResourceManager();
        const auto* material_instance = mesh.getVolumeMaterialInstance();
        auto front_face_material_data = material_instance->cloneForFrontFacePass(createPrePassFragmentShader(),
                                                                                 RenderContext::context().generateId());
        auto back_face_material_data = material_instance->cloneForBackFacePass(createPrePassFragmentShader(),
                                                                               RenderContext::context().generateId());

        result.front_face_technique = front_face_material_data.instance->createTechnique(gpu_resource_manager,
                                                                                         {},
                                                                                         getRenderPass(),
                                                                                         0);
        result.back_face_technique = back_face_material_data.instance->createTechnique(gpu_resource_manager,
                                                                                       {},
                                                                                       getRenderPass(),
                                                                                       1);
        result.subpass_materials.push_back(std::move(front_face_material_data.material));
        result.subpass_material_instance.push_back(std::move(front_face_material_data.instance));
        result.subpass_materials.push_back(std::move(back_face_material_data.material));
        result.subpass_material_instance.push_back(std::move(back_face_material_data.instance));

        std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> additional_texture_bindings;
        std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> subpass_texture_bindings;
        const uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();

        const bool need_distance_field = material_instance->getVolumeMaterial().isRequireDistanceField();
        if (need_distance_field)
        {
            const auto& intensity_image = material_instance->getIntensityImage();
            result.segmentation_threshold = material_instance->getSegmentationThreshold();
            Image distance_field_image(intensity_image.getWidth(),
                                       intensity_image.getHeight(),
                                       intensity_image.getDepth(),
                                       VK_FORMAT_R32_SFLOAT);
            for (uint32_t i = 0; i < back_buffer_size; ++i)
            {

                result.synchronization_objects.emplace_back(SyncObject(getLogicalDevice()));
                result.synchronization_objects.back().createTimelineSemaphore(DistanceFieldFinishedCallback::kSemaphoreName,
                                                                              DistanceFieldFinishedCallback::kReadyValue,
                                                                              DistanceFieldFinishedCallback::kReadyValue - DistanceFieldFinishedCallback::kProcessValue + 1);

                result.synchronization_objects.back().addWaitOperationToGroup(SyncGroups::kExternal,
                                                                              DistanceFieldFinishedCallback::kSemaphoreName,
                                                                              VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                                              DistanceFieldFinishedCallback::kReadyValue);

                result.distance_field_textures.push_back(getTextureFactory().createExternalNoUpload(distance_field_image,
                                                                                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                                    VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                                                    VK_IMAGE_USAGE_SAMPLED_BIT
                                                                                                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                                                                                    | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
                result.distance_field_texture_views.push_back(
                    result.distance_field_textures.back()->createTextureView(
                        Texture::ImageViewData{ },
                        Texture::SamplerData{
                            .mag_filter = VK_FILTER_NEAREST,
                            .min_filter = VK_FILTER_NEAREST,
                            .sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                            .border_color = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE }));
                {
                    cudaChannelFormatDesc channel_format{};
                    assert(distance_field_image.getFormat() == VK_FORMAT_R32_SFLOAT);
                    channel_format.x = 32;
                    channel_format.y = 0;
                    channel_format.z = 0;
                    channel_format.w = 0;
                    channel_format.f = cudaChannelFormatKindFloat;
                    result.distance_field_surface.push_back(
                        std::make_unique<CudaCompute::ExternalSurface>(distance_field_image.getWidth(),
                                                                       distance_field_image.getHeight(),
                                                                       distance_field_image.getDepth(),
                                                                       result.distance_field_textures.back()->getMemoryRequirements().size,
                                                                       channel_format,
                                                                       result.distance_field_textures.back()->getMemoryHandle()));
                }
                {
                    cudaChannelFormatDesc channel_format{};
                    assert(intensity_image.getFormat() == VK_FORMAT_R8G8B8A8_SRGB);
                    channel_format.x = 8;
                    channel_format.y = 8;
                    channel_format.z = 8;
                    channel_format.w = 8;
                    channel_format.f = cudaChannelFormatKindUnsigned;
                    // TODO: When the texture can really change frame by frame it's gonna be a synchronization issue.
                    result.intensity_surface.push_back(
                        std::make_unique<CudaCompute::ExternalSurface>(intensity_image.getWidth(),
                                                                       intensity_image.getHeight(),
                                                                       intensity_image.getDepth(),
                                                                       material_instance->getIntensityTexture().getMemoryRequirements().size,
                                                                       channel_format,
                                                                       material_instance->getIntensityTexture().getMemoryHandle()));
                }
            }
        }
        for (uint32_t i = 0; i < back_buffer_size; ++i)
        {
            subpass_texture_bindings[VolumeShader::MetaDataExtension::kFrontFaceTextureBinding].push_back(_front_face_frame_buffer.texture_views_per_back_buffer[i]->createReference());
            subpass_texture_bindings[VolumeShader::MetaDataExtension::kBackFaceTextureBinding].push_back(_back_face_frame_buffer.texture_views_per_back_buffer[i]->createReference());
            if (need_distance_field)
            {
                additional_texture_bindings[VolumeShader::MetaDataExtension::kDistanceFieldBinding].push_back(result.distance_field_texture_views[i]->createReference());
            }

        }
        result.volume_technique = mesh.getMaterialInstance()->createTechnique(gpu_resource_manager,
                                                                              TextureBindingMap{ std::move(subpass_texture_bindings) },
                                                                              getRenderPass(),
                                                                              2,
                                                                              TextureBindingMap{ std::move(additional_texture_bindings) });

        return result;
    }

    std::vector<VolumeRenderer::AttachmentInfo> VolumeRenderer::reinitializeAttachments(const RenderTarget& render_target)
    {
        auto render_pass_attachments = createFrameBuffersAndAttachments(render_target);
        // Reinitialize existing techniques, because these are using the attachments.
        for (auto& mesh_group : _meshes)
        {
            mesh_group.technique_data = createTechniqueDataFor(*mesh_group.meshes.front());
        }

        return render_pass_attachments;
    }

    std::vector<VolumeRenderer::AttachmentInfo> VolumeRenderer::createFrameBuffersAndAttachments(const RenderTarget& render_target)
    {
        auto& gpu_resource_manager = getWindow().getRenderEngine().getGpuResourceManager();
        const uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();

        initializeFrameBuffers(back_buffer_size, render_target.getImage(0));
        std::vector<AttachmentInfo> render_pass_attachments;
        for (uint32_t i = 0; i < back_buffer_size; ++i)
        {
            AttachmentInfo attachment_info{
                .attachments = {
                    _front_face_frame_buffer.texture_views_per_back_buffer[i].get(),
                    _back_face_frame_buffer.texture_views_per_back_buffer[i].get()
            }
            };
            render_pass_attachments.push_back(std::move(attachment_info));
        }
        return render_pass_attachments;
    }

    void VolumeRenderer::resetResourceStatesOf(Technique& technique, uint32_t image_index)
    {
        for (const auto& uniform_binding : technique.getUniformBindings())
        {
            if (auto texture = uniform_binding->getTextureForFrame(image_index); texture != nullptr)
            {
                ResourceStateMachine::resetStages(*texture);
            }
            if (auto buffer = uniform_binding->getTextureForFrame(image_index); buffer != nullptr)
            {
                ResourceStateMachine::resetStages(*buffer);
            }
        }
    }

}
