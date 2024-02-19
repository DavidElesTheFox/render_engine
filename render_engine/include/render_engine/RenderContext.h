#pragma once

#include <memory>
#include <vector>

#include <volk.h>

#include <render_engine/DeviceLookup.h>
#include <render_engine/RendererFactory.h>

#include <variant>

#include <render_engine/synchronization/SyncObject.h>
namespace RenderEngine
{
    class Device;

    class RenderContext
    {
    public:
        using Garbage = std::variant<SyncObject>;

        struct InitializationInfo
        {
            struct QueueFamilyIndexes
            {
                uint32_t graphics{};
                uint32_t present{};
                uint32_t transfer{};
            };
            std::function<VkPhysicalDevice(const DeviceLookup&)> device_selector;
            std::function<QueueFamilyIndexes(const DeviceLookup::DeviceInfo&)> queue_family_selector;
            std::vector<std::string> enabled_layers;
            std::unique_ptr<RendererFeactory> renderer_factory;
            std::vector<std::string> instance_extensions;
            std::vector<std::string> device_extensions;
            VkApplicationInfo app_info{};
            bool enable_validation_layers{ true };
        };
        // TODO replace ids to generated UUID
        static constexpr uint32_t kEngineReservedIdStart = UINT_MAX - 1'000'000;
        static RenderContext& context();
        static void initialize(InitializationInfo&& info);

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

        [[deprecated("This object is a sign of ownership problem. It's not nice but sometimes a you need a shortcut. "
                     "This is a visible short cut and the goal is that whenever it is used let's remove it soon.")]]
        void addGarbage(Garbage garbage) { _garbage.emplace_back(std::move(garbage)); }
        [[deprecated("This object is a sign of ownership problem. It's not nice but sometimes a you need a shortcut. "
                     "This is a visible short cut and the goal is that whenever it is used let's remove it soon.")]]
        void clearGarbage();
    private:
        struct GarbageData
        {
            explicit GarbageData(Garbage garbage)
                : object(std::move(garbage))
            {}
            Garbage object;
            uint32_t life_count{ 16 };
        };
        static RenderContext& context_impl();
        RenderContext() = default;
        ~RenderContext()
        {
            reset();
        }
        void init(InitializationInfo&& info);
        void initVulkan(const InitializationInfo& info);
        void createDevices(const InitializationInfo& info);
        bool isVulkanInitialized() const { return _instance != nullptr; }
        VkInstance _instance;
        std::vector<std::unique_ptr<Device>> _devices;
        VkDebugUtilsMessengerEXT _debug_messenger;
        void* _renderdoc_api{ nullptr };
        std::unique_ptr<RendererFeactory> _renderer_factory;
        bool _initialized{ false };
        uint32_t _engine_id_counter = kEngineReservedIdStart;
        std::vector<GarbageData> _garbage;
    };
}