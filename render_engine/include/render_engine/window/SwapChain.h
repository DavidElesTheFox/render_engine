#pragma once

#include <volk.h>

#include <render_engine/LogicalDevice.h>

#include <GLFW/glfw3.h>

#include <future>
#include <memory>
#include <mutex>
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
        struct ImageAcquireResult
        {
            uint32_t image_index{ 0 };
            VkResult result{ VK_RESULT_MAX_ENUM };
        };
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
            bool use_background_processing{ false };
        };
        SwapChain(CreateInfo create_info);

        ~SwapChain();
        RenderTarget createRenderTarget() const;

        std::future<ImageAcquireResult> acquireNextImageAsync(uint64_t timeout, VkSemaphore semaphore, VkFence fence);
        ImageAcquireResult acquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence)
        {
            return acquireNextImageAsync(timeout, semaphore, fence).get();
        }
        std::future<void> presentAsync(VkQueue queue, VkPresentInfoKHR&& present_info);
        void present(VkQueue queue, VkPresentInfoKHR&& present_info)
        {
            presentAsync(queue, std::move(present_info)).get();
        }
        void reinit();
        [[ deprecated("Swap chain should not directly accessed") ]]
        auto getSwapChain()
        {
            struct Result
            {
                std::unique_lock<std::mutex> lock;
                VkSwapchainKHR swap_chain;
            };
            return Result{ std::unique_lock(_access_mutex), _details.swap_chain };
        }
    private:
        class BackgroundWorker;

        void resetSwapChain();

        Details _details;
        CreateInfo _create_info;
        std::mutex _access_mutex;
        std::thread _background_thread;
        std::unique_ptr<BackgroundWorker> _background_worker;
    };
}