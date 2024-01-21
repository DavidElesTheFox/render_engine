#include <render_engine/renderers/ImageStreamRenderer.h>

#include <volk.h>

#include <render_engine/RenderContext.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/resources/Technique.h>

#include <cassert>
#include <format>
#include <iostream>

namespace RenderEngine
{
    namespace
    {
        /*
        #version 450

        layout(location = 0) out vec2 fragTexCoord;

        void main() {
            fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
            gl_Position = vec4(fragTexCoord * 2.0f + -1.0f, 0.0f, 1.0f);
        }
        */
        const std::vector<uint32_t> vertex_shader_code =
        {
            0x07230203,0x00010000,0x000d000b,0x0000002c,
            0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,
            0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0008000f,0x00000000,0x00000004,0x6e69616d,
            0x00000000,0x00000009,0x0000000c,0x0000001d,
            0x00030003,0x00000002,0x000001c2,0x000a0004,
            0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,
            0x5f656c79,0x656e696c,0x7269645f,0x69746365,
            0x00006576,0x00080004,0x475f4c47,0x4c474f4f,
            0x6e695f45,0x64756c63,0x69645f65,0x74636572,
            0x00657669,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00060005,0x00000009,0x67617266,
            0x43786554,0x64726f6f,0x00000000,0x00060005,
            0x0000000c,0x565f6c67,0x65747265,0x646e4978,
            0x00007865,0x00060005,0x0000001b,0x505f6c67,
            0x65567265,0x78657472,0x00000000,0x00060006,
            0x0000001b,0x00000000,0x505f6c67,0x7469736f,
            0x006e6f69,0x00070006,0x0000001b,0x00000001,
            0x505f6c67,0x746e696f,0x657a6953,0x00000000,
            0x00070006,0x0000001b,0x00000002,0x435f6c67,
            0x4470696c,0x61747369,0x0065636e,0x00070006,
            0x0000001b,0x00000003,0x435f6c67,0x446c6c75,
            0x61747369,0x0065636e,0x00030005,0x0000001d,
            0x00000000,0x00040047,0x00000009,0x0000001e,
            0x00000000,0x00040047,0x0000000c,0x0000000b,
            0x0000002a,0x00050048,0x0000001b,0x00000000,
            0x0000000b,0x00000000,0x00050048,0x0000001b,
            0x00000001,0x0000000b,0x00000001,0x00050048,
            0x0000001b,0x00000002,0x0000000b,0x00000003,
            0x00050048,0x0000001b,0x00000003,0x0000000b,
            0x00000004,0x00030047,0x0000001b,0x00000002,
            0x00020013,0x00000002,0x00030021,0x00000003,
            0x00000002,0x00030016,0x00000006,0x00000020,
            0x00040017,0x00000007,0x00000006,0x00000002,
            0x00040020,0x00000008,0x00000003,0x00000007,
            0x0004003b,0x00000008,0x00000009,0x00000003,
            0x00040015,0x0000000a,0x00000020,0x00000001,
            0x00040020,0x0000000b,0x00000001,0x0000000a,
            0x0004003b,0x0000000b,0x0000000c,0x00000001,
            0x0004002b,0x0000000a,0x0000000e,0x00000001,
            0x0004002b,0x0000000a,0x00000010,0x00000002,
            0x00040017,0x00000017,0x00000006,0x00000004,
            0x00040015,0x00000018,0x00000020,0x00000000,
            0x0004002b,0x00000018,0x00000019,0x00000001,
            0x0004001c,0x0000001a,0x00000006,0x00000019,
            0x0006001e,0x0000001b,0x00000017,0x00000006,
            0x0000001a,0x0000001a,0x00040020,0x0000001c,
            0x00000003,0x0000001b,0x0004003b,0x0000001c,
            0x0000001d,0x00000003,0x0004002b,0x0000000a,
            0x0000001e,0x00000000,0x0004002b,0x00000006,
            0x00000020,0x40000000,0x0004002b,0x00000006,
            0x00000022,0xbf800000,0x0004002b,0x00000006,
            0x00000025,0x00000000,0x0004002b,0x00000006,
            0x00000026,0x3f800000,0x00040020,0x0000002a,
            0x00000003,0x00000017,0x00050036,0x00000002,
            0x00000004,0x00000000,0x00000003,0x000200f8,
            0x00000005,0x0004003d,0x0000000a,0x0000000d,
            0x0000000c,0x000500c4,0x0000000a,0x0000000f,
            0x0000000d,0x0000000e,0x000500c7,0x0000000a,
            0x00000011,0x0000000f,0x00000010,0x0004006f,
            0x00000006,0x00000012,0x00000011,0x0004003d,
            0x0000000a,0x00000013,0x0000000c,0x000500c7,
            0x0000000a,0x00000014,0x00000013,0x00000010,
            0x0004006f,0x00000006,0x00000015,0x00000014,
            0x00050050,0x00000007,0x00000016,0x00000012,
            0x00000015,0x0003003e,0x00000009,0x00000016,
            0x0004003d,0x00000007,0x0000001f,0x00000009,
            0x0005008e,0x00000007,0x00000021,0x0000001f,
            0x00000020,0x00050050,0x00000007,0x00000023,
            0x00000022,0x00000022,0x00050081,0x00000007,
            0x00000024,0x00000021,0x00000023,0x00050051,
            0x00000006,0x00000027,0x00000024,0x00000000,
            0x00050051,0x00000006,0x00000028,0x00000024,
            0x00000001,0x00070050,0x00000017,0x00000029,
            0x00000027,0x00000028,0x00000025,0x00000026,
            0x00050041,0x0000002a,0x0000002b,0x0000001d,
            0x0000001e,0x0003003e,0x0000002b,0x00000029,
            0x000100fd,0x00010038
        };
        /*
        #version 450

        layout(location = 0) in vec2 fragTexCoord;

        layout(location = 0) out vec4 outColor;

        layout(binding = 0) uniform sampler2D texSampler;

        void main() {
            outColor = texture(texSampler, fragTexCoord);
        }
        */
        const std::vector<uint32_t> fragment_shader_code =
        { 0x07230203,0x00010000,0x000d000b,0x00000014,
 0x00000000,0x00020011,0x00000001,0x0006000b,
 0x00000001,0x4c534c47,0x6474732e,0x3035342e,
 0x00000000,0x0003000e,0x00000000,0x00000001,
 0x0007000f,0x00000004,0x00000004,0x6e69616d,
 0x00000000,0x00000009,0x00000011,0x00030010,
 0x00000004,0x00000007,0x00030003,0x00000002,
 0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,
 0x70635f45,0x74735f70,0x5f656c79,0x656e696c,
 0x7269645f,0x69746365,0x00006576,0x00080004,
 0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,
 0x69645f65,0x74636572,0x00657669,0x00040005,
 0x00000004,0x6e69616d,0x00000000,0x00050005,
 0x00000009,0x4374756f,0x726f6c6f,0x00000000,
 0x00050005,0x0000000d,0x53786574,0x6c706d61,
 0x00007265,0x00060005,0x00000011,0x67617266,
 0x43786554,0x64726f6f,0x00000000,0x00040047,
 0x00000009,0x0000001e,0x00000000,0x00040047,
 0x0000000d,0x00000022,0x00000000,0x00040047,
 0x0000000d,0x00000021,0x00000000,0x00040047,
 0x00000011,0x0000001e,0x00000000,0x00020013,
 0x00000002,0x00030021,0x00000003,0x00000002,
 0x00030016,0x00000006,0x00000020,0x00040017,
 0x00000007,0x00000006,0x00000004,0x00040020,
 0x00000008,0x00000003,0x00000007,0x0004003b,
 0x00000008,0x00000009,0x00000003,0x00090019,
 0x0000000a,0x00000006,0x00000001,0x00000000,
 0x00000000,0x00000000,0x00000001,0x00000000,
 0x0003001b,0x0000000b,0x0000000a,0x00040020,
 0x0000000c,0x00000000,0x0000000b,0x0004003b,
 0x0000000c,0x0000000d,0x00000000,0x00040017,
 0x0000000f,0x00000006,0x00000002,0x00040020,
 0x00000010,0x00000001,0x0000000f,0x0004003b,
 0x00000010,0x00000011,0x00000001,0x00050036,
 0x00000002,0x00000004,0x00000000,0x00000003,
 0x000200f8,0x00000005,0x0004003d,0x0000000b,
 0x0000000e,0x0000000d,0x0004003d,0x0000000f,
 0x00000012,0x00000011,0x00050057,0x00000007,
 0x00000013,0x0000000e,0x00000012,0x0003003e,
 0x00000009,0x00000013,0x000100fd,0x00010038 }
        ;
    }
    ImageStreamRenderer::ImageStreamRenderer(IWindow& window,
                                             ImageStream& image_stream,
                                             const RenderTarget& render_target,
                                             uint32_t back_buffer_size, bool last_renderer)
        try : SingleColorOutputRenderer(window)
        , _image_stream(image_stream)
        , _image_cache(image_stream.getImageDescription().width,
                       image_stream.getImageDescription().height,
                       image_stream.getImageDescription().format)
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

        auto& transfare_engine = window.getTransferEngine();
        TextureFactory texture_factory(transfare_engine,
                                       { transfare_engine.getQueueFamilyIndex(), window.getRenderEngine().getQueueFamilyIndex() },
                                       window.getDevice().getPhysicalDevice(),
                                       window.getDevice().getLogicalDevice());


        for (uint32_t i = 0; i < back_buffer_size; ++i)
        {
            _texture_container.push_back(texture_factory.createNoUpload(_image_cache,
                                                                        VK_IMAGE_ASPECT_COLOR_BIT,
                                                                        VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        }
        {
            Shader::MetaData fragment_metadata{ .samplers = {{0, {.binding = 0, .update_frequency = Shader::MetaData::UpdateFrequency::PerFrame }}} };
            Shader::MetaData vertex_metadata{ }; // no attachments. Does not need any input just glVertexId
            auto vertex_shader = std::make_unique<Shader>(std::span(vertex_shader_code), vertex_metadata);
            auto fragment_shader = std::make_unique<Shader>(std::span(fragment_shader_code), fragment_metadata);
            const uint32_t material_id = RenderContext::context().generateId();
            _fullscreen_material = std::make_unique<Material>(std::move(vertex_shader),
                                                              std::move(fragment_shader),
                                                              Material::CallbackContainer{},
                                                              material_id);

            Texture::SamplerData sampler_data{};


            std::unordered_map<int32_t, std::vector<std::unique_ptr<ITextureView>>> texture_bindings;
            const int32_t input_binding = 0;
            for (uint32_t i = 0; i < back_buffer_size; ++i)
            {
                texture_bindings[input_binding].push_back(std::make_unique<TextureView>(*_texture_container[i],
                                                                                        Texture::ImageViewData{},
                                                                                        sampler_data,
                                                                                        getWindow().getDevice().getPhysicalDevice(),
                                                                                        getLogicalDevice()));
            }


            _material_instance = std::make_unique<MaterialInstance>(*_fullscreen_material,
                                                                    TextureBindingMap{ std::move(texture_bindings) },
                                                                    MaterialInstance::CallbackContainer{},
                                                                    material_id);

            _technique = _material_instance->createTechnique(getWindow().getRenderEngine().getGpuResourceManager(),
                                                             {},
                                                             getRenderPass(),
                                                             0);
        }
    }
    catch (const std::exception&)
    {
        destroyRenderOutput();
        destroy();
    }

    void ImageStreamRenderer::onFrameBegin(uint32_t image_index)
    {
        if (const auto& texture = _texture_container[image_index]; texture != nullptr)
        {
            ResourceStateMachine::resetStages(*texture);
        }

        if (const auto& texture = _texture_container[image_index]; texture != nullptr)
        {
            ResourceStateMachine::resetStages(*texture);
        }
    }

    SyncOperations ImageStreamRenderer::getSyncOperations(uint32_t image_index)
    {
        if (skipDrawCall(image_index))
        {
            return {};
        }
        auto& texture = _texture_container[image_index];
        auto it = _upload_data.find(texture.get());
        if (it == _upload_data.end())
        {
            return {};
        }

        return it->second.synchronization_object_out.getDefaultOperations();
    }

    void ImageStreamRenderer::destroy() noexcept
    {
        for (auto& upload_data : _upload_data | std::ranges::views::values)
        {
            upload_data.synchronization_object = SynchronizationObject::CreateEmpty(getLogicalDevice());
            upload_data.synchronization_object_out = SynchronizationObject::CreateEmpty(getLogicalDevice());
        }
    }

    void ImageStreamRenderer::draw(uint32_t swap_chain_image_index)
    {
        static std::vector<uint8_t> image_data;
        image_data.clear();
        _image_stream >> image_data;
        if (image_data.empty() == false)
        {
            _image_cache.setData(image_data);
        }
        VkDevice logical_device = getLogicalDevice();

        {
            auto& upload_texture = _texture_container[swap_chain_image_index];
            auto it = _upload_data.find(upload_texture.get());
            if (it == _upload_data.end())
            {
                SynchronizationObject sync_objcet = SynchronizationObject::CreateWithFence(logical_device, 0);

                sync_objcet.createSemaphore("copy-finished");
                sync_objcet.addSignalOperation("copy-finished",
                                               VK_PIPELINE_STAGE_2_COPY_BIT);

                SynchronizationObject sync_objcet_out = SynchronizationObject::CreateEmpty(logical_device);

                sync_objcet_out.addWaitOperation(sync_objcet,
                                                 "copy-finished",
                                                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT); // TODO add as pipeline dependency

                auto inflight_data = upload_texture->upload(_image_cache,
                                                            sync_objcet.getDefaultOperations(),
                                                            getWindow().getTransferEngine(),
                                                            getWindow().getRenderEngine().getQueueFamilyIndex());
                _upload_data.insert(std::make_pair(upload_texture.get(),
                                                   UploadData(std::move(inflight_data),
                                                              std::move(sync_objcet),
                                                              std::move(sync_objcet_out))));
            }
            else
            {
                vkWaitForFences(logical_device, 1, it->second.synchronization_object.getDefaultOperations().getFence(), VK_TRUE, UINT64_MAX);
                vkResetFences(logical_device, 1, it->second.synchronization_object.getDefaultOperations().getFence());

                it->second.inflight_data = upload_texture->upload(_image_cache,
                                                                  it->second.synchronization_object.getDefaultOperations(),
                                                                  getWindow().getTransferEngine(),
                                                                  getWindow().getRenderEngine().getQueueFamilyIndex());
            }
        }
        _draw_call_recorded = false;
        auto& render_texture = _texture_container[swap_chain_image_index];
        if (_upload_data.contains(render_texture.get()))
        {
            _draw_call_recorded = true;
            FrameData& frame_data = getFrameData(swap_chain_image_index);

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;



            auto render_area = getRenderArea();
            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = getRenderPass();
            render_pass_info.framebuffer = getFrameBuffer(swap_chain_image_index);
            render_pass_info.renderArea = render_area;

            VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            render_pass_info.clearValueCount = 1;
            render_pass_info.pClearValues = &clearColor;

            if (vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }
            vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            auto& technique = _technique;

            vkCmdBindPipeline(frame_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->getPipeline());

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
            auto descriptor_sets = technique->collectDescriptorSets(swap_chain_image_index);

            if (descriptor_sets.empty() == false)
            {
                vkCmdBindDescriptorSets(frame_data.command_buffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        technique->getPipelineLayout(),
                                        0,
                                        descriptor_sets.size(),
                                        descriptor_sets.data(), 0, nullptr);
            }

            // Draw nothing just 3 'empty' vertexes trick is in the vertex shader
            vkCmdDraw(frame_data.command_buffer, 3, 1, 0, 0);
            vkCmdEndRenderPass(frame_data.command_buffer);

            if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

    }
    bool ImageStreamRenderer::skipDrawCall(uint32_t frame_number) const
    {
        return _draw_call_recorded == false;
    }
}