#include <render_engine/synchronization/ThreadingInfo.h>

#include <render_engine/RenderContext.h>
#include <render_engine/Topic.h>

#include <cassert>

namespace RenderEngine
{
    uint32_t ThreadingInfo::registerThread(std::thread::id thread_id)
    {
        std::lock_guard lock(_mapping_mutex);
        auto it = std::ranges::find_if(_mapping,
                                       [&](const auto& id) { return id == thread_id; });
        if (it == _mapping.end())
        {
            return registerNewThread(thread_id);
        }
        else
        {
            _ref_counters[thread_id]++;
            return static_cast<uint32_t>(std::distance(_mapping.begin(), it));
        }
    }

    uint32_t ThreadingInfo::registerNewThread(std::thread::id thread_id)
    {
        _ref_counters[thread_id] = 1;

        auto it = std::ranges::find_if(_mapping,
                                       [](const auto& id) { return id == std::nullopt; });
        if (it == _mapping.end())
        {
            assert(std::ranges::find_if(_mapping, [&](const auto& id) { return id == thread_id; }) == _mapping.end());
            _mapping.emplace_back(thread_id);
            return static_cast<uint32_t>(_mapping.size() - 1);
        }
        else
        {
            *it = thread_id;
            return static_cast<uint32_t>(std::distance(_mapping.begin(), it));
        }
    }

    void ThreadingInfo::unregisterThread(std::thread::id thread_id)
    {
        std::lock_guard lock(_mapping_mutex);
        auto ref_count = _ref_counters.find(thread_id);
        assert(ref_count->first == thread_id);
        ref_count->second--;

        if (ref_count->second == 0)
        {
            auto it = std::ranges::find_if(_mapping, [&](const auto& id) { return id == thread_id; });
            *it = std::nullopt;
            _ref_counters.erase(ref_count);
        }
    }
    bool ThreadingInfo::isThreadRegistered(std::thread::id thread_id)
    {
        std::lock_guard lock(_mapping_mutex);
        return _ref_counters.find(thread_id) != _ref_counters.end();
    }
    uint32_t ThreadingInfo::getThreadIndex(std::thread::id thread_id)
    {
        std::lock_guard lock(_mapping_mutex);
        auto it = std::ranges::find_if(_mapping, [&](const auto& id) { return id == thread_id; });
        assert(it != _mapping.end());

        return static_cast<uint32_t>(std::distance(_mapping.begin(), it));
    }
}