#pragma once

#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

namespace RenderEngine
{
    class ThreadingInfo
    {
    public:

        struct RegistrationScope
        {
            RegistrationScope(ThreadingInfo* info, std::thread::id id)
                : _info(info)
                , _id(id)
            {
                _index = _info->registerThread(id);
            }
            ~RegistrationScope()
            {
                _info->unregisterThread(_id);
            }
            uint32_t getIndex() const { return _index; }
        private:
            ThreadingInfo* _info;
            std::thread::id _id;
            uint32_t _index;
        };

        uint32_t registerThread(std::thread::id);
        void unregisterThread(std::thread::id);
        bool isThreadRegistered(std::thread::id);
        uint32_t getThreadIndex(std::thread::id);

        [[nodiscard]]
        RegistrationScope getScope(std::thread::id thread_id)
        {
            return RegistrationScope{ this, thread_id };
        }
    private:
        uint32_t registerNewThread(std::thread::id);

        mutable std::mutex _mapping_mutex;
        std::vector<std::optional<std::thread::id>> _mapping;
        std::unordered_map<std::thread::id, uint32_t> _ref_counters;
    };
}