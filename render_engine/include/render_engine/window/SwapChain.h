#pragma once

#include <volk.h>

#include <GLFW/glfw3.h>
#include <vector>

namespace RenderEngine
{
    class RenderTarget;
    class SwapChain
    {
    public:
        struct Details
        {
            VkSwapchainKHR swap_chain{ VK_NULL_HANDLE };
            std::vector<VkImage> images;
            std::vector<VkImageView> image_views;
            VkFormat image_format{ VK_FORMAT_UNDEFINED };
            VkExtent2D extent{};
            VkSurfaceKHR surface{ VK_NULL_HANDLE };
        };

        struct CreateInfo
        {
            GLFWwindow* window{ nullptr };
            VkInstance instance{ VK_NULL_HANDLE };
            VkPhysicalDevice physical_device{ VK_NULL_HANDLE };
            VkDevice logical_device{ VK_NULL_HANDLE };
            VkSurfaceKHR surface{ VK_NULL_HANDLE };
            uint32_t graphics_family_index{ 0 };
            uint32_t present_family_index{ 0 };
            uint32_t back_buffer_size{ 0 };
        };
        SwapChain(CreateInfo create_info);

        ~SwapChain();
        const Details& getDetails() const { return _details; }
        RenderTarget createRenderTarget() const;
        void reinit();
    private:
        void resetSwapChain();
        Details _details;
        CreateInfo _create_info;
    };
}