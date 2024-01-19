#pragma once

#include <cuda_runtime_api.h>

#include <cstdint>

namespace RenderEngine::CudaCompute
{
    class ExternalSurface
    {
    public:
        ExternalSurface(uint32_t width,
                        uint32_t height,
                        uint32_t depth,
                        uint64_t memory_size,
                        cudaChannelFormatDesc channel_format,
                        int32_t memory_handle);

        ExternalSurface(const ExternalSurface&) = delete;
        ExternalSurface& operator=(const ExternalSurface&) = delete;
        ExternalSurface(ExternalSurface&&) = delete;
        ExternalSurface& operator=(ExternalSurface&&) = delete;

        uint32_t getWidth() const { return _width; }
        uint32_t getHeight() const { return _height; }
        uint32_t getDepth() const { return _depth; }

        ~ExternalSurface();

        cudaSurfaceObject_t getSurface() const { return _surface; }
    private:
        uint32_t _width{};
        uint32_t _height{};
        uint32_t _depth{};

        cudaArray_t _array{ };
        cudaExternalMemory_t _external_memory{  };
        cudaMipmappedArray_t _mipmapped_representation{};
        cudaSurfaceObject_t _surface{ };
    };
}