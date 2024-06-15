#include <render_engine/ParallelRenderEngine.h>

#include <render_engine/containers/Views.h>
#include <render_engine/rendergraph/TaskflowBuilder.h>

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
                                               uint32_t back_buffer_count)
        : _device(device)
        , _render_context(std::move(render_context))
        , _present_context(std::move(present_context))
        , _transfer_context(std::move(transfer_context))
        , _gpu_resource_manager(std::make_unique<GpuResourceManager>(device.getPhysicalDevice(), device.getLogicalDevice(), back_buffer_count, kMaxNumOfResources))
        , _transfer_engine(std::make_unique<TransferEngine>(_transfer_context))
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

        const uint32_t back_buffer_size = _gpu_resource_manager->getBackBufferSize();
        uint32_t thread_count = back_buffer_size;
        thread_count = 1;


        _rendering_processes.reserve(thread_count);
        _skeleton = std::move(render_graph);

        _sync_objects.clear();
        std::vector<SyncObject*> non_const_sync_objects;
        for (uint32_t i = 0; i < back_buffer_size; ++i)
        {
            std::unique_ptr<SyncObject> sync_object = std::make_unique<SyncObject>(_device.getLogicalDevice());
            for (const std::variant<RenderGraph::TimelineSemaphore, RenderGraph::BinarySemaphore>& semaphore_definition : _skeleton->getSemaphoreDefinitions())
            {
                std::visit(overloaded(
                    [&](const RenderGraph::TimelineSemaphore& semaphore) { sync_object->createTimelineSemaphore(semaphore.getName(), semaphore.getInitialValue(), semaphore.getValueRange()); },
                    [&](const RenderGraph::BinarySemaphore& semaphore) { sync_object->createSemaphore(semaphore.getName()); }),
                    semaphore_definition);
            }
            non_const_sync_objects.push_back(sync_object.get());
            _sync_objects.push_back(std::move(sync_object));
        }

        for (uint32_t i = 0; i < thread_count; ++i)
        {
            auto rendering_process = std::make_unique<RenderingProcess>(non_const_sync_objects);

            rendering_process->task_flow = task_flow_builder.createTaskflow(*_skeleton,
                                                                            rendering_process->execution_context,
                                                                            _device.getLogicalDevice(),
                                                                            non_const_sync_objects);
            _rendering_processes.push_back(std::move(rendering_process));
        }
    }

    void ParallelRenderEngine::render()
    {
        if (_skeleton == nullptr)
        {
            return;
        }
        auto& current_process = _rendering_processes[_render_call_count % _rendering_processes.size()];

        if (current_process->calling_token.valid())
        {
            current_process->calling_token.wait();
        }
        if (current_process->execution_context.isDrawCallRecorded())
        {
            current_process->execution_context.clearRenderTargetIndex();
            current_process->execution_context.setDrawCallRecorded(false);
        }

        current_process->calling_token = current_process->executor.run(current_process->task_flow);

        _render_call_count++;

    }
}