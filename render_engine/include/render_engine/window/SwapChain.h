#pragma once

#include <volk.h>

#include <render_engine/LogicalDevice.h>

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

namespace RenderEngine
{
    class RenderTarget;
    class Texture;
    class ITextureView;
    class Device;
    class SwapChain
    {
    public:
        struct Details
        {
            VkSwapchainKHR swap_chain{ VK_NULL_HANDLE };
            std::vector<std::unique_ptr<Texture>> textures;
            std::vector< std::unique_ptr<ITextureView>> texture_views;
            VkFormat image_format{ VK_FORMAT_UNDEFINED };
            VkExtent2D extent{};
            VkSurfaceKHR surface{ VK_NULL_HANDLE };
            ~Details();
            Details() = default;
            Details(Details&&) = default;
            Details(const Details&) = delete;

            Details& operator=(Details&&) = default;
            Details& operator=(const Details&) = delete;
        };

        struct CreateInfo
        {
            GLFWwindow* window{ nullptr };
            VkInstance instance{ VK_NULL_HANDLE };
            VkPhysicalDevice physical_device{ VK_NULL_HANDLE };
            LogicalDevice* logical_device{ nullptr };
            VkSurfaceKHR surface{ VK_NULL_HANDLE };
            Device* device{ nullptr };
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