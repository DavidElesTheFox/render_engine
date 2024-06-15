#pragma once

#include <volk.h>

#include <deque>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace RenderEngine
{
    class SyncLogbook
    {
    public:
        enum class SemaphoreType
        {
            Binary,
            Timeline
        };
        explicit SyncLogbook(uint32_t stack_max_size)
            : _stack_max_size(stack_max_size)
        {}

        void registerSemaphore(std::string name, SemaphoreType type, void* handler, std::string sync_object_name);
        void signaledSemaphoreFromHost(void* handler, uint64_t value);
        void imageAcquire(void* handler);
        void usedAtSubmitForSignal(void* handler, VkPipelineStageFlagBits2 stage);
        void usedAtSubmitForWait(void* handler, VkPipelineStageFlagBits2 stage);
        void usedAtPresentForWait(void* handler);
        friend std::ostream& operator<<(std::ostream& os, const SyncLogbook& logbook);
        std::string toString() const;
    private:
        enum class Operation
        {
            Wait,
            WaitAtPresent,
            Signal,
            SignalFromHost,
            ImageAcquire
        };
        struct StageInfo
        {
            VkPipelineStageFlagBits2 stage_flag;
        };
        struct SemaphoreEntry
        {
            std::string name;
            SemaphoreType type;
            void* handler{ nullptr };
            std::string sync_object_name;
        };
        struct StackLine
        {
            Operation operation;
            void* handler{ nullptr };
            std::optional<uint64_t> value{ std::nullopt };
            std::optional<StageInfo> stage_info{ std::nullopt };
        };
        friend std::ostream& operator<<(std::ostream& os, const Operation&);
        friend std::ostream& operator<<(std::ostream& os, const SemaphoreEntry&);
        friend std::ostream& operator<<(std::ostream& os, const StackLine&);
        friend std::ostream& operator<<(std::ostream& os, const StageInfo&);

        void shrink();

        std::unordered_map<void*, SemaphoreEntry> _semaphores;
        std::deque<StackLine> _stack;
        mutable std::shared_mutex _semaphore_mutex;
        mutable std::shared_mutex _stack_mutex;
        uint32_t _stack_max_size{ 1 };
    };
}