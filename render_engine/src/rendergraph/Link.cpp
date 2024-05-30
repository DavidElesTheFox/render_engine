#include <render_engine/rendergraph/Link.h>

#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/rendergraph/GraphVisitor.h>

#include <format>


namespace RenderEngine::RenderGraph
{
    Link::PipelineConnection::PipelineConnection(std::string semaphore_name,
                                                 VkPipelineStageFlags2 signal_stage,
                                                 VkPipelineStageFlags2 wait_stage,
                                                 std::optional<uint64_t> value)
        : semaphore_name(std::move(semaphore_name))
        , signal_stage(signal_stage)
        , wait_stage(wait_stage)
        , value(value)
    {}

    Link::ExternalConnection::ExternalConnection(std::string signaled_semaphore_name,
                                                 VkPipelineStageFlags2 wait_stage,
                                                 std::optional<uint64_t> value)
        : signaled_semaphore_name(std::move(signaled_semaphore_name))
        , wait_stage(wait_stage)
        , value(value)
    {}

    void Link::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }


    void Link::forEachConnections(const std::function<void(const PipelineConnection&)>& callback_pipeline,
                                  const std::function<void(const ExternalConnection&)>& callback_external) const
    {
        for (const auto& connection : _connections)
        {
            // clang-format off
            std::visit(overloaded{
                           [&](const PipelineConnection& c)
                           {
                                callback_pipeline(c);
                           },
                           [&](const ExternalConnection& c)
                           {
                                callback_external(c);
                           }
                       }, connection);
            // clang-format on
        }
    }

}