#include <render_engine/renderers/UIRenderer.h>

#include <render_engine/Device.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/window/Window.h>

#include <fstream>
#include <stdexcept>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace
{
    using namespace RenderEngine;

    VkRenderPass createRenderPass(const RenderTarget& render_target, VkDevice logical_device, bool first_renderer)
    {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = render_target.getImageFormat();
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = first_renderer ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = first_renderer ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.finalLayout = render_target.getLayout();

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

        if (vkCreateRenderPass(logical_device, &renderPassInfo, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
        return result;
    }

    VkDescriptorPool createDescriptorPool(VkDevice logical_device)
    {
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
        if (vkCreateDescriptorPool(logical_device, &pool_info, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
        return result;
    }


}

namespace RenderEngine
{
    std::set<ImGuiContext*> UIRenderer::kInvalidContexts;

    UIRenderer::UIRenderer(Window& window,
                           const RenderTarget& render_target,
                           uint32_t back_buffer_size,
                           bool first_renderer)
        : _window(window)
    {
        _imgui_context_during_init = ImGui::GetCurrentContext();
        _imgui_context = ImGui::CreateContext();
        ImGui::SetCurrentContext(_imgui_context);

        ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance)
                                       {
                                           return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
                                       }, &window.getDevice().getVulkanInstance());

        ImGui_ImplGlfw_InitForVulkan(_window.getWindowHandle(), true);


        auto logical_device = window.getDevice().getLogicalDevice();

        try
        {
            _descriptor_pool = createDescriptorPool(logical_device);
            _render_pass = createRenderPass(render_target, logical_device, first_renderer);

            _back_buffer.resize(back_buffer_size);
            _render_area.offset = { 0, 0 };
            _render_area.extent = render_target.getExtent();
            createFrameBuffers(render_target);
            _command_pool = _window.getRenderEngine().getCommandPoolFactory().getCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
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
                SynchronizationPrimitives synchronization_primitives = SynchronizationPrimitives::CreateWithFence(logical_device);

                // Use the render queue to upload the data, while queue ownership transfer cannot be defined for ImGui.
                auto inflight_data = _window.getTransferEngine().transfer(synchronization_primitives,
                                                                          [transfer_queue_index = _window.getTransferEngine().getQueueFamilyIndex(), render_queue_index = window.getRenderEngine().getQueueFamilyIndex()](VkCommandBuffer command_buffer)
                                                                          {
                                                                              ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
                                                                          },
                                                                          _window.getRenderEngine().getRenderQueue());

                vkWaitForFences(logical_device, 1, &synchronization_primitives.on_finished_fence, VK_TRUE, UINT64_MAX);

                vkDestroyFence(logical_device, synchronization_primitives.on_finished_fence, nullptr);
            }
            ImGui_ImplVulkan_DestroyFontUploadObjects();

        }
        catch (const std::runtime_error&)
        {
            auto logical_device = window.getDevice().getLogicalDevice();
            vkDestroyCommandPool(logical_device, _command_pool.command_pool, nullptr);

            for (auto framebuffer : _frame_buffers)
            {
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
        vkDestroyCommandPool(logical_device, _command_pool.command_pool, nullptr);

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

        for (auto framebuffer : _frame_buffers)
        {
            vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
        }
    }
    void UIRenderer::beforeReinit()
    {
        resetFrameBuffers();
    }
    void UIRenderer::finalizeReinit(const RenderTarget& render_target)
    {
        createFrameBuffers(render_target);
        _render_area.offset = { 0, 0 };
        _render_area.extent = render_target.getExtent();;
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

        if (vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
        {
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

        if (vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
        {
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

    void UIRenderer::createFrameBuffers(const RenderTarget& render_target)
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        _frame_buffers.resize(render_target.getImageViews().size());
        for (uint32_t i = 0; i < _frame_buffers.size(); ++i)
        {
            if (createFrameBuffer(render_target, i) == false)
            {
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroyFramebuffer(logical_device, _frame_buffers[j], nullptr);
                }
                _frame_buffers.clear();
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    bool UIRenderer::createFrameBuffer(const RenderTarget& render_target, uint32_t frame_buffer_index)
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        VkImageView attachments[] = {
                render_target.getImageViews()[frame_buffer_index]
        };
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = render_target.getWidth();
        framebuffer_info.height = render_target.getHeight();
        framebuffer_info.layers = 1;

        return vkCreateFramebuffer(logical_device, &framebuffer_info, nullptr, &_frame_buffers[frame_buffer_index]) == VK_SUCCESS;
    }

    void UIRenderer::createCommandBuffer()
    {
        auto logical_device = _window.getDevice().getLogicalDevice();

        for (FrameData& frame_data : _back_buffer)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = _command_pool.command_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(logical_device, &allocInfo, &frame_data.command_buffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }
    }
}