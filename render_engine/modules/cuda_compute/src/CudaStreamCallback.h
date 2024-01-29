#pragma once

#include <cuda_runtime_api.h>

namespace RenderEngine::CudaCompute
{
    void cudaStreamCallbackImpl(cudaStream_t stream, cudaError_t status, void* userData);
}