#pragma once

#include <cstdint>

#include <cuda_runtime_api.h>

#include <random>

namespace RenderEngine
{
    namespace CudaCompute
    {
        class DistanceFieldKernel
        {
        public:
            struct KernelParameters
            {
                dim3 grid_size{};
                dim3 block_size{};
            };
            static cudaArray_t allocateInputMemory(uint32_t width, uint32_t height, uint32_t depth);
            static cudaArray_t allocateOutputMemory(uint32_t width, uint32_t height, uint32_t depth);
            static void freeMemory(cudaArray_t device_memory);

            explicit DistanceFieldKernel(const KernelParameters& kernel_parameters,
                                         cudaStream_t cuda_stream = 0);

            void run(const cudaSurfaceObject_t d_input_data,
                     cudaSurfaceObject_t d_output_data,
                     uint32_t segmentation_threshold,
                     float epsilon_distance);

        private:
            KernelParameters _kernel_parameters;

            uint32_t _shift{ 0xffffffff };
            cudaStream_t _cuda_stream;
        };
    }
}