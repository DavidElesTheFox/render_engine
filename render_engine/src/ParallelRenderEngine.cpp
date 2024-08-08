#include <render_engine/ParallelRenderEngine.h>

#include <render_engine/containers/Views.h>
#include <render_engine/rendergraph/TaskflowBuilder.h>
#include <render_engine/synchronization/Topic.h>

#include <optick.h>

namespace RenderEngine
{
    namespace
    {
        constexpr uint32_t kMaxNumOfResources = 10;
    }
    using namespace RenderGraph;

    ParallelRenderEngine::ParallelRenderEngine(Device& device,
                                               std::shared_ptr<SingleShotCommandContext>&& transfer_context,
                                               std::shared_ptr<CommandContext>&& render_context,
                                               std::shared_ptr<CommandContext>&& present_context,
                                               Description description)
        : _device(device)
        , _render_context(std::move(render_context))
        , _present_context(std::move(present_context))
        , _transfer_context(std::move(transfer_context))
        , _description(std::move(description))
        , _gpu_resource_manager(std::make_unique<GpuResourceManager>(device.getPhysicalDevice(), device.getLogicalDevice(), _description.backbuffer_count, kMaxNumOfResources))
        , _transfer_engine(std::make_unique<TransferEngine>(_transfer_context))
        , _task_flow_executor()
    {}

    RenderGraphBuilder ParallelRenderEngine::createRenderGraphBuilder(std::string graph_name)
    {
        return RenderGraphBuilder(std::move(graph_name), _render_context, _present_context, _transfer_context);
    }
    void ParallelRenderEngine::setRenderGraph(std::unique_ptr<RenderGraph::Graph> render_graph)
    {
        if (_skeleton != nullptr)
        {
            throw std::runtime_error("Renderer has already a graph");
        }
        TaskflowBuilder task_flow_builder;

        _rendering_processes.reserve(_description.parallel_frame_count);
        _skeleton = std::move(render_graph);


        for (uint32_t i = 0; i < _description.parallel_frame_count; ++i)
        {
            auto sync_object = createSyncObjectFromGraph(*_skeleton, std::format("ExecutionContext-{:d}", i));
            SyncObject* non_const_sync_object = sync_object.get();
            auto rendering_process = std::make_unique<RenderingProcess>(std::move(sync_object));

            rendering_process->task_flow = task_flow_builder.createTaskflow(*_skeleton,
                                                                            rendering_process->execution_context,
                                                                            _device.getLogicalDevice(),
                                                                            non_const_sync_object);
            _rendering_processes.push_back(std::move(rendering_process));
        }
    }

    std::unique_ptr<SyncObject> ParallelRenderEngine::createSyncObjectFromGraph(const RenderGraph::Graph& graph, std::string name) const
    {
        std::unique_ptr<SyncObject> sync_object = std::make_unique<SyncObject>(_device.getLogicalDevice(), name);
        for (const std::variant<RenderGraph::TimelineSemaphore, RenderGraph::BinarySemaphore>& semaphore_definition : graph.getSemaphoreDefinitions())
        {
            std::visit(overloaded(
                [&](const RenderGraph::TimelineSemaphore& semaphore) { sync_object->createTimelineSemaphore(semaphore.getName(), semaphore.getInitialValue(), semaphore.getValueRange()); },
                [&](const RenderGraph::BinarySemaphore& semaphore) { sync_object->createSemaphore(semaphore.getName()); }),
                semaphore_definition);
        }
        return sync_object;
    }

    void ParallelRenderEngine::waitIdle()
    {
        for (const auto& current_process : _rendering_processes)
        {
            if (current_process->calling_token.valid())
            {
                current_process->calling_token.wait();
            }
        }
    }

    void ParallelRenderEngine::render()
    {
        OPTICK_FRAME("Main");

        if (_skeleton == nullptr)
        {
            return;
        }
        // TODO: Actually here we could continue one of the available processes.
        auto& current_process = _rendering_processes[_render_call_count % _rendering_processes.size()];

        if (current_process->calling_token.valid())
        {
            current_process->calling_token.wait();
        }
        if (current_process->execution_context.isDrawCallRecorded())
        {
            current_process->execution_context.clearPoolIndex();
            current_process->execution_context.setDrawCallRecorded(false);
            current_process->execution_context.clearSubmitTrackersPool();
        }
        current_process->execution_context.setCurrentFrameNumber(_render_call_count);
        current_process->calling_token = _task_flow_executor.run(current_process->task_flow);

        auto& debugger = RenderContext::context().getDebugger();
        debugger.print(Debug::Topics::Synchronization{}, "Synchronization Log @{:d}: \n{:s}\n",
                       _render_call_count,
                       debugger.getSyncLogbook().toString());
        _render_call_count++;

    }
}