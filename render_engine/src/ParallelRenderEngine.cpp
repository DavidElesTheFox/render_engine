#include <render_engine/ParallelRenderEngine.h>

#include <render_engine/containers/Views.h>
#include <render_engine/rendergraph/TaskflowBuilder.h>
#include <render_engine/rendergraph/Topic.h>
#include <render_engine/synchronization/SyncFeedbackService.h>
#include <render_engine/synchronization/Topic.h>

#include <optick.h>

namespace RenderEngine
{
    namespace
    {
        constexpr uint32_t kMaxNumOfResources = 10;
    }
    using namespace RenderGraph;

    class ParallelRenderEngine::SyncService
    {
    public:

        void add(std::unique_ptr<SyncObject> sync_object)
        {
            _sync_objects.push_back(std::move(sync_object));
        }
        std::vector<const SyncObject*> collectConstSyncObjects() const
        {
            using namespace std::views;
            using namespace views;
            return all(_sync_objects) | to_raw_pointer | to<std::vector<const SyncObject*>>();
        }
        std::vector<SyncObject*> collectSyncObjects()
        {
            using namespace std::views;
            using namespace views;
            return all(_sync_objects) | to_raw_pointer | to<std::vector<SyncObject*>>();
        }

        SyncFeedbackService* getFeedbackService()
        {
            return _feedback_service.get();
        }
    private:
        std::unique_ptr<SyncFeedbackService> _feedback_service{ std::make_unique<SyncFeedbackService>() };
        std::vector<std::unique_ptr<SyncObject>> _sync_objects;
    };

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
        , _sync_service(std::make_unique<SyncService>())
        , _available_backbuffers(_description.backbuffer_count)
        , _task_flow_executor(_description.parallel_frame_count)
    {
        std::ranges::iota(_available_backbuffers, 0);
    }

    ParallelRenderEngine::~ParallelRenderEngine() = default;

    RenderGraphBuilder ParallelRenderEngine::createRenderGraphBuilder(std::string graph_name)
    {
        return RenderGraphBuilder(std::move(graph_name), _render_context, _present_context, _transfer_context);
    }
    void ParallelRenderEngine::setRenderGraph(std::unique_ptr<RenderGraph::Graph> render_graph)
    {
        using namespace std::views;
        using namespace views;
        if (_skeleton != nullptr)
        {
            throw std::runtime_error("Renderer has already a graph");
        }
        TaskflowBuilder task_flow_builder;

        _rendering_processes.reserve(_description.parallel_frame_count);
        _skeleton = std::move(render_graph);

        std::vector<SyncObject*> sync_object_pointers;
        for (uint32_t i = 0; i < _description.backbuffer_count; ++i)
        {
            auto sync_object = createSyncObjectFromGraph(*_skeleton, std::format("ExecutionContext-{:d}", i));
            sync_object_pointers.push_back(sync_object.get());
            _sync_service->add(std::move(sync_object));
        }

        for (uint32_t i = 0; i < _description.backbuffer_count; ++i)
        {
            auto rendering_process = std::make_unique<RenderingProcess>(all(sync_object_pointers) | as_const | to<std::vector<const SyncObject*>>(), _sync_service->getFeedbackService(), i);

            rendering_process->task_flow = task_flow_builder.createTaskflow(*_skeleton,
                                                                            rendering_process->execution_context,
                                                                            _device.getLogicalDevice(),
                                                                            sync_object_pointers[i],
                                                                            *_sync_service->getFeedbackService());
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

    uint32_t ParallelRenderEngine::popAvailableBackbufferId()
    {
        const uint32_t threading_capacity_limit = _description.backbuffer_count - _description.parallel_frame_count;

        std::unique_lock lock(_available_backbuffers_mutex);
        if (_available_backbuffers.size() <= threading_capacity_limit)
        {
            const bool ok = _available_backbuffers_condition.wait_for(lock,
                                                                      _description._frame_timeout,
                                                                      [&] { return _available_backbuffers.size() > threading_capacity_limit; });
            if (ok == false)
            {
                throw std::runtime_error("Frame time out occurred. Within the frame timeout limit rendering commands couldn't be issued");
            }
        }
        uint32_t id = _available_backbuffers.front();
        _available_backbuffers.pop_front();
        return id;
    }
    void ParallelRenderEngine::pushAvailableBackbufferId(uint32_t id)
    {
        std::unique_lock lock(_available_backbuffers_mutex);
        _available_backbuffers.push_back(id);
        lock.unlock();
        _available_backbuffers_condition.notify_one();
    }
    void ParallelRenderEngine::render()
    {
        OPTICK_FRAME("Main");

        if (_skeleton == nullptr)
        {
            return;
        }
        auto& debugger = RenderContext::context().getDebugger();
        const uint32_t available_backbuffer_id = popAvailableBackbufferId();

        auto& current_process = _rendering_processes[available_backbuffer_id];

        if (current_process->execution_context.isDrawCallRecorded())
        {
            //current_process->execution_context.clearPoolIndex();
            current_process->execution_context.setDrawCallRecorded(false);
        }

        current_process->execution_context.getSyncFeedbackService().clearFences();

        current_process->execution_context.setCurrentFrameNumber(_render_call_count);

        current_process->calling_token = _task_flow_executor.run(current_process->task_flow,
                                                                 [=]
                                                                 {
                                                                     pushAvailableBackbufferId(available_backbuffer_id);
                                                                 });
        debugger.print(Debug::Topics::Synchronization{}, "Synchronization Log @{:d}: \n{:s}\n",
                       _render_call_count,
                       debugger.getSyncLogbook().toString());
        debugger.print(Debug::Topics::RenderGraphExecution{}, "Pipeline started for frame: {:d} on: {:d} [thread: {}]",
                       _render_call_count,
                       available_backbuffer_id,
                       std::this_thread::get_id());
        _render_call_count++;

    }
}