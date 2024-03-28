#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/rendergraph/Graph.h>

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
                                    Job::ExecutionContext& execution_context,
                                    LogicalDevice& logical_device,
                                    uint32_t backbuffer_count);
    };
}
