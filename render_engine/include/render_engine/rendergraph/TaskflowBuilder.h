#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/rendergraph/Graph.h>
#include <render_engine/rendergraph/Job.h>

#include <memory>

#pragma warning(push, 0)
#include <taskflow/taskflow.hpp>
#pragma warning(pop)

namespace RenderEngine::RenderGraph
{

    class TaskflowBuilder
    {
    public:
        TaskflowBuilder() = default;

        tf::Taskflow createTaskflow(Graph& graph,
                                    ExecutionContext& execution_context,
                                    LogicalDevice& logical_device,
                                    const std::vector<SyncObject*>& sync_objects);
    };
}
