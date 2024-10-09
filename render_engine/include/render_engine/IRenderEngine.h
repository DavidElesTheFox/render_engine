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
        IRenderEngine(IRenderEngine&&) = delete;
        IRenderEngine(const IRenderEngine&) = delete;

        IRenderEngine& operator=(IRenderEngine&&) = delete;
        IRenderEngine& operator=(const IRenderEngine&) = delete;
        virtual ~IRenderEngine() = default;

        virtual GpuResourceManager& getGpuResourceManager() = 0;
        virtual uint32_t getBackBufferSize() const = 0;
        virtual TransferEngine& getTransferEngine() = 0;
        virtual TransferEngine& getTransferEngineOnRenderQueue() = 0;
        virtual CommandBufferContext& getCommandBufferContext() = 0;
        virtual Device& getDevice() = 0;
    };
}