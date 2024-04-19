#include <render_engine/rendergraph/Link.h>

#include <render_engine/containers/VariantOverloaded.h>
#include <render_engine/rendergraph/GraphVisitor.h>

#include <format>


namespace RenderEngine::RenderGraph
{
    Link::PipelineConnection::PipelineConnection(DependentName semaphore_name,
                                                 VkPipelineStageFlags2 signal_stage,
                                                 VkPipelineStageFlags2 wait_stage,
                                                 std::optional<uint64_t> value)
        : semaphore_name(std::move(semaphore_name))
        , signal_stage(signal_stage)
        , wait_stage(wait_stage)
        , value(value)
    {}

    Link::ExternalConnection::ExternalConnection(DependentName signaled_semaphore_name,
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

    //void Link::addSyncOperationsForSource(SyncObject& sync_object, uint32_t back_buffer_size) const
    //{
    //    for (const auto& connection : _connections)
    //    {
    //        for (uint32_t render_target_index : std::views::iota(0u, back_buffer_size))
    //        {
    //            // clang-format off
    //            std::visit(overloaded{
    //                           [&](const PipelineConnection& c)
    //                           {
    //                                if (c.value == std::nullopt)
    //                                {
    //                                    sync_object.addSignalOperationToGroup(syncGroup(render_target_index),
    //                                                                          c.semaphore_name.getName(render_target_index),
    //                                                                          c.signal_stage);
    //                                }
    //                                else
    //                                {
    //                                    sync_object.addSignalOperationToGroup(syncGroup(render_target_index),
    //                                                                          c.semaphore_name.getName(render_target_index),
    //                                                                          c.signal_stage,
    //                                                                          *c.value);
    //                                
    //                           [&](const ExternalConnection& c)
    //                           {
    //                                sync_object.addSemaphoreForHostOperations(syncGroup(render_target_index), c.signaled_semaphore_name.getName(render_target_index));
    //                           }
    //                       }, connection);
    //            // clang-format on
    //        }
    //    }}
    //                           },
    //}

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
    //void Link::addSyncOperationsForDestination(SyncObject& sync_object, uint32_t back_buffer_size) const
    //{
    //    for (const auto& connection : _connections)
    //    {
    //        for (uint32_t render_target_index : std::views::iota(0u, back_buffer_size))
    //        {
    //            // clang-format off
    //            std::visit(overloaded{
    //                           [&](const PipelineConnection& c)
    //                           {
    //                                if (c.value == std::nullopt)
    //                                {
    //                                    sync_object.addWaitOperationToGroup(syncGroup(render_target_index),
    //                                                                        c.semaphore_name.getName(render_target_index),
    //                                                                        c.wait_stage);
    //                                }
    //                           },
    //                           [&](const ExternalConnection& c)
    //                           {
    //                                sync_object.addWaitOperationToGroup(syncGroup(render_target_index),
    //                                                                    c.signaled_semaphore_name.getName(render_target_index),
    //                                                                    c.wait_stage);
    //                           }
    //                       }, connection);
    //            // clang-format on
    //        }
    //    }
    //}
}