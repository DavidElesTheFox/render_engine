#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/Device.h>
#include <render_engine/QueueSubmitTracker.h>
#include <render_engine/synchronization/SyncOperations.h>

#include <functional>
namespace RenderEngine
{
    // TODO: Transfer engine is copyable so far, probably it needs to be changed when batch upload is implemented.
    class TransferEngine
    {
    public:

        explicit TransferEngine(CommandBufferContext transfer_context);

        void transfer(const SyncOperations& sync_operations,
                      std::function<void(VkCommandBuffer)> record_transfer_command,
                      QueueSubmitTracker* queue_submit_tracker);

        const std::shared_ptr<SingleShotCommandBufferFactory>& getCommandBufferFactory() const { return _transfer_context.getSingleShotFactory(); }
        std::shared_ptr<SingleShotCommandBufferFactory>& getCommandBufferFactory() { return _transfer_context.getSingleShotFactory(); }
    private:
        CommandBufferContext _transfer_context;
    };
}