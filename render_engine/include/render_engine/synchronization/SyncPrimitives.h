#pragma once

#include <volk.h>

#include <string>
#include <unordered_map>

#include <render_engine/LogicalDevice.h>

namespace RenderEngine
{
    class SyncPrimitives
    {
    public:
        static SyncPrimitives CreateWithFence(LogicalDevice& logical_device, VkFenceCreateFlags flags);
        static SyncPrimitives CreateEmpty(LogicalDevice& logical_device)
        {
            return SyncPrimitives{ logical_device };
        }

        ~SyncPrimitives();
        SyncPrimitives(SyncPrimitives&& o);
        SyncPrimitives(const SyncPrimitives& o) = delete;
        SyncPrimitives& operator=(SyncPrimitives&& o);
        SyncPrimitives& operator=(const SyncPrimitives& o) = delete;

        void createSemaphore(std::string name);

        void createTimelineSemaphore(std::string name, uint64_t initial_value, uint64_t timeline_width);

        const VkSemaphore& getSemaphore(const std::string& name) const
        {
            return _semaphore_map.at(name);
        }

        VkFence getFence() const { return _fence; }

        LogicalDevice& getLogicalDevice() const { return *_logical_device; }

        uint64_t getTimelineOffset(const std::string& name)
        {
            return _timeline_data.at(name).timeline_offset;
        }

        uint64_t stepTimeline(const std::string& name);

        bool hasSemaphore(const std::string& name) const { return _semaphore_map.contains(name); }
        bool hasTimelineSemaphore(const std::string& name) const { return _timeline_data.contains(name); }
    private:
        struct TimelineSemaphoreData
        {
            uint64_t timeline_width{ 0 };
            uint64_t timeline_offset{ 0 };
            uint64_t initial_value{ 0 };
        };
        explicit SyncPrimitives(LogicalDevice& logical_device) :
            _logical_device(&logical_device)
        {}


        std::unordered_map<std::string, VkSemaphore> _semaphore_map;
        std::unordered_map<std::string, TimelineSemaphoreData> _timeline_data;
        VkFence _fence{ VK_NULL_HANDLE };

        LogicalDevice* _logical_device{ nullptr };
    };
}