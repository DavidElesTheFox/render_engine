#pragma once

#include <render_engine/CommandContext.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/TransferEngine.h>

namespace RenderEngine
{
    // TODO remove it: There is no need more than one render engine
    class IRenderEngine
    {
    public:
        IRenderEngine() = default;
        virtual ~IRenderEngine() = default;

        virtual GpuResourceManager& getGpuResourceManager() = 0;
        virtual uint32_t getBackBufferSize() const = 0;
        virtual TransferEngine& getTransferEngine() = 0;
        virtual CommandContext& getCommandContext() = 0;
        virtual SingleShotCommandContext& getTransferCommandContext() = 0;
        virtual Device& getDevice() = 0;
    };
}