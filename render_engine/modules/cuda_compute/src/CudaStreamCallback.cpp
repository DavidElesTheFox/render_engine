#include <CudaStreamCallback.h>

#include <render_engine/cuda_compute/IComputeCallback.h>

namespace RenderEngine::CudaCompute
{
    void cudaStreamCallbackImpl(cudaStream_t stream, cudaError_t status, void* userData)
    {
        auto callback = reinterpret_cast<IComputeCallback*>(userData);
        callback->call();
    }
}