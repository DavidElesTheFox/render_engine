#include <render_engine/renderers/UIRenderer.h>

#include <render_engine/Device.h>
#include <render_engine/resources/RenderTarget.h>
#include <render_engine/window/Window.h>

#include <fstream>
#include <stdexcept>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>



namespace RenderEngine
{
    namespace
    {
        // Copy from imgui_impl_vulkan.cpp
#define IMGUI_VULKAN_FUNC_MAP(IMGUI_VULKAN_FUNC_MAP_MACRO) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkAllocateCommandBuffers) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkAllocateDescriptorSets) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkAllocateMemory) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkBindBufferMemory) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkBindImageMemory) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdBindDescriptorSets) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdBindIndexBuffer) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdBindPipeline) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdBindVertexBuffers) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdCopyBufferToImage) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdDrawIndexed) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdPipelineBarrier) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdPushConstants) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdSetScissor) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCmdSetViewport) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateBuffer) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateCommandPool) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateDescriptorSetLayout) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateFence) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateFramebuffer) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateGraphicsPipelines) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateImage) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateImageView) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreatePipelineLayout) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateRenderPass) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateSampler) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateSemaphore) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateShaderModule) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkCreateSwapchainKHR) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyBuffer) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyCommandPool) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyDescriptorSetLayout) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyFence) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyFramebuffer) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyImage) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyImageView) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyPipeline) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyPipelineLayout) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyRenderPass) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroySampler) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroySemaphore) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroyShaderModule) \
    /*IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroySurfaceKHR) */\
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroySwapchainKHR) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDeviceWaitIdle) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkFlushMappedMemoryRanges) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkFreeCommandBuffers) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkFreeDescriptorSets) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkFreeMemory) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetBufferMemoryRequirements) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetImageMemoryRequirements) \
    /*IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceMemoryProperties)*/ \
    /*IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)*/ \
    /*IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceSurfaceFormatsKHR)*/ \
    /*IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceSurfacePresentModesKHR)*/ \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetSwapchainImagesKHR) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkMapMemory) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkUnmapMemory) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkUpdateDescriptorSets)

#define IMGUI_VULKAN_INSTANCE_FUNC_MAP(IMGUI_VULKAN_FUNC_MAP_MACRO) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkDestroySurfaceKHR)\
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceMemoryProperties) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    IMGUI_VULKAN_FUNC_MAP_MACRO(vkGetPhysicalDeviceSurfacePresentModesKHR) 

        PFN_vkVoidFunction loadVulkanFunction(const char* function_name, void* user_data)
        {
            LogicalDevice& logical_device = *reinterpret_cast<LogicalDevice*>(user_data);
#define _MAP_FUNCTION(name) if(strcmp(function_name, ###name) == 0) return reinterpret_cast<PFN_vkVoidFunction>(logical_device->name);
            IMGUI_VULKAN_FUNC_MAP(_MAP_FUNCTION);
#undef __MAP_FUNCTION

#define _MAP_FUNCTION(name) if(strcmp(function_name, ###name) == 0) return reinterpret_cast<PFN_vkVoidFunction>(name);
            IMGUI_VULKAN_INSTANCE_FUNC_MAP(_MAP_FUNCTION);
#undef __MAP_FUNCTION
            return nullptr;
        }


        VkRenderPass createRenderPass(const RenderTarget& render_target, LogicalDevice& logical_device, bool first_renderer)
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

            if (logical_device->vkCreateRenderPass(*logical_device, &renderPassInfo, nullptr, &result) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create render pass!");
            }
            return result;
        }

        VkDescriptorPool createDescriptorPool(LogicalDevice& logical_device)
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
            if (logical_device->vkCreateDescriptorPool(*logical_device, &pool_info, nullptr, &result) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create descriptor pool!");
            }
            return result;
        }
    }
    UIRenderer::ImGuiGlobals& UIRenderer::getGlobals()
    {
        static ImGuiGlobals globals{};
        return globals;
    }


    UIRenderer::UIRenderer(Window& window,
                           const RenderTarget& render_target,
                           uint32_t back_buffer_size,
                           bool first_renderer)
        : SingleColorOutputRenderer(window)
    {
        _imgui_context_during_init = ImGui::GetCurrentContext();
        _imgui_context = ImGui::CreateContext();
        ImGui::SetCurrentContext(_imgui_context);
        _window_handle = window.getWindowHandle();

        VkDevice prev_device = getGlobals().loadedDevicePrototypes.exchange(*getLogicalDevice());
        if (prev_device != *getLogicalDevice())
        {
            loadVulkanPrototypes();
        }
        ImGui_ImplGlfw_InitForVulkan(_window_handle, true);


        auto& logical_device = window.getDevice().getLogicalDevice();

        try
        {
            auto render_pass = createRenderPass(render_target, logical_device, first_renderer);
            initializeRendererOutput(render_target, render_pass, back_buffer_size);
            _descriptor_pool = createDescriptorPool(logical_device);

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = window.getDevice().getVulkanInstance();
            init_info.PhysicalDevice = window.getDevice().getPhysicalDevice();
            init_info.Device = *logical_device;
            init_info.Queue = window.getRenderEngine().getCommandContext().getQueue();
            init_info.DescriptorPool = _descriptor_pool;
            init_info.MinImageCount = back_buffer_size;
            init_info.ImageCount = back_buffer_size;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;



            ImGui_ImplVulkan_Init(&init_info, getRenderPass());
            {
                SyncObject sync_object = SyncObject::CreateWithFence(logical_device, 0);

                // Using the render engine's transfer capability
                window.getRenderEngine().getTransferEngine().transfer(sync_object.getOperationsGroup(SyncGroups::kInternal),
                                                                      [](VkCommandBuffer command_buffer)
                                                                      {
                                                                          ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
                                                                      });

                logical_device->vkWaitForFences(*logical_device, 1, sync_object.getOperationsGroup(SyncGroups::kInternal).getFence(), VK_TRUE, UINT64_MAX);

            }
            ImGui_ImplVulkan_DestroyFontUploadObjects();

        }
        catch (const std::runtime_error&)
        {
            destroyRenderOutput();
            logical_device->vkDestroyDescriptorPool(*getLogicalDevice(), _descriptor_pool, nullptr);
        }
    }

    UIRenderer::~UIRenderer()
    {
        ImGui::SetCurrentContext(_imgui_context);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        registerDeletedContext(_imgui_context);
        ImGui::DestroyContext(_imgui_context);
        if (isValidContext(_imgui_context_during_init))
        {
            ImGui::SetCurrentContext(_imgui_context_during_init);
        }
        getLogicalDevice()->vkDestroyDescriptorPool(*getLogicalDevice(), _descriptor_pool, nullptr);
    }

    void UIRenderer::draw(const VkFramebuffer& frame_buffer, FrameData& frame_data)
    {
        if (_on_gui == nullptr)
        {
            return;
        }
        const int focused = glfwGetWindowAttrib(_window_handle, GLFW_FOCUSED);

        if (focused)
        {
            if (ImGui::GetCurrentContext() != _imgui_context)
            {
                ImGui::SetCurrentContext(_imgui_context);
            }

            VkDevice prev_device = getGlobals().loadedDevicePrototypes.exchange(*getLogicalDevice());
            if (prev_device != *getLogicalDevice())
            {
                loadVulkanPrototypes();
            }
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (getLogicalDevice()->vkBeginCommandBuffer(frame_data.command_buffer, &begin_info) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        auto renderer_marker = _performance_markers.createMarker(frame_data.command_buffer, "UIRenderer");

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = getRenderPass();
        render_pass_info.framebuffer = frame_buffer;
        render_pass_info.renderArea = getRenderArea();

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        getLogicalDevice()->vkCmdBeginRenderPass(frame_data.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        if (focused)
        {
            _on_gui();
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame_data.command_buffer);
        }
        getLogicalDevice()->vkCmdEndRenderPass(frame_data.command_buffer);

        if (getLogicalDevice()->vkEndCommandBuffer(frame_data.command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void UIRenderer::registerDeletedContext(ImGuiContext* context)
    {
        getGlobals().invalidContexts.insert(context);
    }

    bool UIRenderer::isValidContext(ImGuiContext* context)
    {
        return getGlobals().invalidContexts.contains(context) == false;
    }
    void UIRenderer::loadVulkanPrototypes()
    {
        const bool functions_loaded = ImGui_ImplVulkan_LoadFunctions(&loadVulkanFunction, &getLogicalDevice());
        assert(functions_loaded);
    }

}