#pragma once

#include <volk.h>

#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>
namespace RenderEngine
{
    class Window;
    class RenderEngine;
    class OffScreenWindow;
    class TransferEngine;
    class TextureFactory;

    class Device
    {
    public:
        explicit Device(VkInstance instance,
                        VkPhysicalDevice physical_device,
                        uint32_t queue_family_index_graphics,
                        uint32_t queue_family_index_presentation,
                        uint32_t queue_family_index_transfer,
                        const std::vector<const char*>& device_extensions,
                        const std::vector<const char*>& validation_layers);
        Device(const Device&) = delete;
        Device(Device&&) = delete;

        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) = delete;

        ~Device();
        std::unique_ptr<Window> createWindow(std::string_view name, uint32_t back_buffer_size);
        std::unique_ptr<OffScreenWindow> createOffScreenWindow(std::string_view name, size_t back_buffer_size);
        std::unique_ptr<RenderEngine> createRenderEngine(size_t back_buffer_size);
        std::unique_ptr<TransferEngine> createTransferEngine();
        std::unique_ptr<TextureFactory> createTextureFactory(TransferEngine& transfer_engine,
                                                             std::set<uint32_t> compatible_queue_family_indexes);

        VkDevice getLogicalDevice() { return _logical_device; }
        VkPhysicalDevice getPhysicalDevice() { return _physical_device; }
        VkInstance& getVulkanInstance() { return _instance; }
    private:
        void destroy() noexcept;

        VkInstance _instance;
        VkPhysicalDevice _physical_device;
        VkDevice _logical_device;
        uint32_t _queue_family_present = 0;
        uint32_t _queue_family_graphics = 0;
        uint32_t _queue_family_transfer = 0;
    };
}
