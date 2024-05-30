#pragma once

#include <volk.h>

#include <render_engine/DeviceLookup.h>
#include <render_engine/LogicalDevice.h>

#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>


namespace RenderEngine
{
    namespace CudaCompute
    {
        class CudaDevice;
    }
    class Window;
    class RenderEngine;
    class ParallelRenderEngine;
    class OffScreenWindow;
    class TransferEngine;
    class TextureFactory;
    class DataTransferScheduler;
    class LogicalDevice;
    class SyncOperations;

    class Device;
    class PerformanceMarkerFactory
    {
    public:
        class Marker
        {
            friend class PerformanceMarkerFactory;

        public:
            Marker(const Marker&) = delete;
            Marker(Marker&& o) noexcept
                : _command_buffer(o._command_buffer)
            {
                o._command_buffer = VK_NULL_HANDLE;
            }

            Marker& operator=(const Marker&) = delete;
            Marker& operator=(Marker&& o) noexcept
            {
                std::swap(_command_buffer, o._command_buffer);
                return *this;
            }

            ~Marker();
            void start(const std::string_view& name);
            void finish();
        private:
            explicit Marker(VkCommandBuffer command_buffer)
                : _command_buffer(command_buffer)
            {}
            VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
        };
        // TODO link it with a device and check requirements
        explicit PerformanceMarkerFactory() = default;

        Marker createMarker(VkCommandBuffer command_buffer, const std::string_view& name)
        {
            Marker result{ command_buffer };
            result.start(name);
            return result;
        }
    };

    class Device
    {
    public:
        class DataTransferContext
        {
        public:
            DataTransferContext(std::unique_ptr<TransferEngine> transfer_engine,
                                std::set<uint32_t> queue_family_indexes,
                                VkPhysicalDevice physical_device,
                                LogicalDevice& logical_device);

            ~DataTransferContext();
            DataTransferScheduler& getScheduler() { return *_scheduler; }
            TextureFactory& getTextureFactory() { return *_texture_factory; }
            void synchronizeScheduler(SyncOperations sync_operations);
            void destroy();
        private:
            std::unique_ptr<DataTransferScheduler> _scheduler;
            std::unique_ptr<TransferEngine> _transfer_engine;
            std::unique_ptr<TextureFactory> _texture_factory;
        };
        Device(VkInstance instance,
               VkPhysicalDevice physical_device,
               uint32_t queue_family_index_graphics,
               uint32_t queue_family_index_presentation,
               uint32_t queue_family_index_transfer,
               const std::vector<const char*>& device_extensions,
               const std::vector<const char*>& validation_layers,
               DeviceLookup::DeviceInfo device_info);
        Device(const Device&) = delete;
        Device(Device&&) = delete;

        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) = delete;

        ~Device();
        std::unique_ptr<Window> createWindow(std::string_view name, uint32_t back_buffer_size);
        std::unique_ptr<Window> createParallelWindow(std::string_view name, uint32_t back_buffer_size);
        std::unique_ptr<OffScreenWindow> createOffScreenWindow(uint32_t back_buffer_size);
        std::unique_ptr<RenderEngine> createRenderEngine(uint32_t back_buffer_size);
        std::unique_ptr<ParallelRenderEngine> createParallelRenderEngine(uint32_t back_buffer_size);
        std::unique_ptr<TransferEngine> createTransferEngine();

        LogicalDevice& getLogicalDevice() { return _logical_device; }
        VkPhysicalDevice getPhysicalDevice() { return _physical_device; }
        VkInstance& getVulkanInstance() { return _instance; }

        CudaCompute::CudaDevice& getCudaDevice() const { return *_cuda_device; }
        TextureFactory& getTextureFactory() { return _staging_area.getTextureFactory(); }

        bool hasCudaDevice() const { return _cuda_device != nullptr; }

        void waitIdle();

        void synchronizeStagingArea(SyncOperations syncOperations);
        DataTransferContext& getDataTransferContext() { return _staging_area; }
    private:
        void destroy() noexcept;

        VkInstance _instance;
        VkPhysicalDevice _physical_device;
        LogicalDevice _logical_device;
        uint32_t _queue_family_present = 0;
        uint32_t _queue_family_graphics = 0;
        uint32_t _queue_family_transfer = 0;
        std::unique_ptr<CudaCompute::CudaDevice> _cuda_device;
        DeviceLookup::DeviceInfo _device_info;
        DataTransferContext _staging_area;
    };
}
