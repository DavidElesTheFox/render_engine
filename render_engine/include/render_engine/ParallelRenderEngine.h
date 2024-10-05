#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/IRenderEngine.h>
#include <render_engine/rendergraph/Graph.h>
#include <render_engine/rendergraph/RenderGraphBuilder.h>
#include <render_engine/window/IWindow.h>

#pragma warning(push, 0)
#include <taskflow/taskflow.hpp>
#pragma warning(pop)

#include <tuple>
#include <format>
namespace RenderEngine
{
    class ParallelRenderEngine : public IRenderEngine
    {
        static inline const auto kEndNodeTimeout = std::chrono::milliseconds{ 10000 };

    public:
        struct Description
        {
            // TODO: Add constructor and ensure that parallel_frame_count <= backbuffer_count.
        /*
         * The problem : Having 20 threads with 1 frame buffer.If the 2nd frame is finished first we cannot wait here
         * because if we are waiting here then there will be no available swap chain image for the 1st frame
         */
            uint32_t backbuffer_count{ 0 };
            uint32_t parallel_frame_count{ 0 };
            std::chrono::milliseconds _frame_timeout{ 1'000 };
        };

        static inline const std::string kEndNodeName = "End";

        ParallelRenderEngine(Device& device,
                             CommandBufferContext render_command_buffer_context,
                             CommandBufferContext present_command_buffer_context,
                             TransferEngine transfer_engine,
                             TransferEngine transfer_engine_on_render_queue,
                             Description description);
        ~ParallelRenderEngine() override;
        RenderGraph::RenderGraphBuilder createRenderGraphBuilder(std::string graph_name);
        void setRenderGraph(std::unique_ptr<RenderGraph::Graph> render_graph);

        void render();

        GpuResourceManager& getGpuResourceManager() override { return *_gpu_resource_manager; }
        uint32_t getBackBufferSize() const override { return _gpu_resource_manager->getBackBufferSize(); }
        TransferEngine& getTransferEngine() override { return _transfer_engine; }
        TransferEngine& getTransferEngineOnRenderQueue() override { return _transfer_engine_on_render_queue; }
        CommandBufferContext& getCommandBufferContext() override { return _render_command_buffer_context; }
        Device& getDevice() override { return _device; }
        void waitIdle();
    private:
        struct RenderingProcess
        {
            RenderingProcess(std::vector<const SyncObject*> sync_object,
                             SyncFeedbackService* feedback_service,
                             uint32_t id)
                : execution_context(std::move(sync_object), feedback_service, id)
            {}

            RenderingProcess(RenderingProcess&& o) noexcept = delete;
            RenderingProcess(const RenderingProcess&) noexcept = delete;

            RenderingProcess& operator=(RenderingProcess&&) noexcept = delete;
            RenderingProcess& operator=(const RenderingProcess&) noexcept = delete;

            RenderGraph::ExecutionContext execution_context;
            tf::Taskflow task_flow;
            tf::Future<void> calling_token;
        };

        class SyncService;

        std::unique_ptr<SyncObject> createSyncObjectFromGraph(const RenderGraph::Graph& graph, std::string name) const;
        uint32_t popAvailableBackbufferId();
        void pushAvailableBackbufferId(uint32_t id);

        Device& _device;
        CommandBufferContext _render_command_buffer_context;
        CommandBufferContext _present_command_buffer_context;
        Description _description;

        std::unique_ptr<RenderGraph::Graph> _skeleton;
        std::unique_ptr<GpuResourceManager> _gpu_resource_manager;
        TransferEngine _transfer_engine;
        TransferEngine _transfer_engine_on_render_queue;
        std::unique_ptr<SyncService> _sync_service;
        std::vector<std::unique_ptr<RenderingProcess>> _rendering_processes;
        uint64_t _render_call_count{ 0 };
        mutable std::mutex _available_backbuffers_mutex;
        std::condition_variable _available_backbuffers_condition;
        std::deque<uint32_t> _available_backbuffers;

        tf::Executor _task_flow_executor;

    };
}