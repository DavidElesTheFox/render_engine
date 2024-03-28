#include <render_engine/rendergraph/Link.h>

#include <render_engine/rendergraph/GraphVisitor.h>

#include <format>

namespace RenderEngine::RenderGraph
{
    void Link::accept(GraphVisitor& visitor)
    {
        visitor.visit(this);
    }

    void Link::addSyncOperationsForSource(SyncObject& sync_object, uint32_t back_buffer_size) const
    {
        bool changed = false;
        for (size_t i = 0; i < _connections.size(); ++i)
        {
            const std::string semaphore_name = std::vformat("{:s}_{:d}", std::make_format_args(_from_node->getName(), i));
            if (sync_object.getPrimitives().hasSemaphore(semaphore_name) == false)
            {
                sync_object.createSemaphore(semaphore_name);
            }

            if (_connections[i].signal_stage == std::nullopt)
            {
                assert(_connections[i].signaled_semaphore_name != std::nullopt);
                sync_object.addSemaphoreForHostOperations(syncGroup(i), *_connections[i].signaled_semaphore_name);
                continue;
            }

            sync_object.addSignalOperationToGroup(syncGroup(i),
                                                  semaphore_name,
                                                  *_connections[i].signal_stage);
            changed = true;
        }
        if (changed == false)
        {
            return;
        }
    }

    void Link::addSyncOperationsForDestination(SyncObject& sync_object, uint32_t back_buffer_size) const
    {
        for (uint32_t render_target_index = 0; render_target_index < back_buffer_size; ++render_target_index)
        {
            const std::string semaphore_name = std::vformat("{:s}_{:d}", std::make_format_args(_from_node->getName(), render_target_index));
            for (size_t i = 0; i < _connections.size(); ++i)
            {
                if (sync_object.getPrimitives().hasSemaphore(semaphore_name) == false)
                {
                    sync_object.createSemaphore(semaphore_name);
                }
                sync_object.addWaitOperationToGroup(syncGroup(i),
                                                    semaphore_name,
                                                    _connections[i].wait_stage);
            }
        }
    }
}