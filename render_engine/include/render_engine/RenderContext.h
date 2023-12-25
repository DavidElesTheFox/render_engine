#pragma once

#include <memory>
#include <vector>

#include <volk.h>

#include <render_engine/RendererFactory.h>


namespace RenderEngine
{
    class Device;

    class RenderContext
    {

    public:
        // TODO replace ids to generated UUID
        static constexpr uint32_t kEngineReservedIdStart = UINT_MAX - 1'000'000;
        static RenderContext& context();
        static void initialize(const std::vector<const char*>& validation_layers, std::unique_ptr<RendererFeactory> renderer_factory);

        Device& getDevice(size_t index) const
        {
            return *_devices[index];
        }
        void reset();
        void* getRenderdocApi();
        RendererFeactory& getRendererFactory()
        {
            return *_renderer_factory;
        }
        uint32_t generateId() { return _engine_id_counter++; }
    private:
        static RenderContext& context_impl();
        RenderContext() = default;
        ~RenderContext()
        {
            reset();
        }
        void init(const std::vector<const char*>& validation_layers, std::unique_ptr<RendererFeactory> renderer_factory);
        void initVulkan(const std::vector<const char*>& validation_layers);
        void createDevices(const std::vector<const char*>& validation_layers);
        bool isVulkanInitialized() const { return _instance != nullptr; }
        VkInstance _instance;
        std::vector<std::unique_ptr<Device>> _devices;
        VkDebugUtilsMessengerEXT _debug_messenger;
        void* _renderdoc_api{ nullptr };
        std::unique_ptr<RendererFeactory> _renderer_factory;
        bool _initialized{ false };
        uint32_t _engine_id_counter = kEngineReservedIdStart;
    };
}