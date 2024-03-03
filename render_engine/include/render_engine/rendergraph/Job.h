#pragma once

#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>

namespace RenderEngine::RenderGraph
{
    class Job
    {
    public:
        class ExecutionContext
        {
        public:
            ExecutionContext() = default;

            ExecutionContext(const ExecutionContext&) = delete;
            ExecutionContext(ExecutionContext&&) = default;

            ExecutionContext& operator=(const ExecutionContext&) = delete;
            ExecutionContext& operator=(ExecutionContext&&) = default;

            uint32_t getRenderTargetIndex() const;
            void setRenderTargetIndex(uint32_t index);

            void reset();
        private:
            std::optional<uint32_t> _render_target_index{ std::nullopt };
            mutable std::shared_mutex _render_target_index_mutex;
        };


        Job(std::string name, std::function<void(ExecutionContext&)> job)
            : _name(std::move(name))
            , _job(std::move(job))
        {}

        Job(Job&&) = delete;
        Job(const Job&) = delete;

        Job& operator=(Job&&) = delete;
        Job& operator=(const Job&) = delete;

        const std::string& getName() const { return _name; }
        void execute(ExecutionContext&) noexcept;
    private:
        std::string _name;
        std::function<void(ExecutionContext&)> _job;
    };
}