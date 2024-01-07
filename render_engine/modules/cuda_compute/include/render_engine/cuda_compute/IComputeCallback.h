#pragma once

namespace RenderEngine::CudaCompute
{
    class IComputeCallback
    {
    public:
        virtual ~IComputeCallback() = default;

        virtual void call() = 0;
    protected:
        IComputeCallback() = default;
    };
}