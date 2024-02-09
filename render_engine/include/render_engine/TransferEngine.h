#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/Device.h>
#include <render_engine/synchronization/SyncOperations.h>

#include <functional>
namespace RenderEngine
{
    class TransferEngine
    {
    public:

        explicit TransferEngine(std::shared_ptr<CommandContext>&& transfer_context);

        void transfer(const SyncOperations& sync_operations,
                      std::function<void(VkCommandBuffer)> record_transfer_command);

        const CommandContext& getTransferContext() const { return *_transfer_context; }
        CommandContext& getTransferContext() { return *_transfer_context; }
    private:
        std::shared_ptr<CommandContext> _transfer_context;
    };
}