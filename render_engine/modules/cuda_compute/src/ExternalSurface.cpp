#include <render_engine/cuda_compute/ExternalSurface.h>

#include <cassert>

#define CUDA_CHECKED_CALL(exp) {cudaError e = (exp); assert(e == cudaSuccess && #exp);}

namespace RenderEngine::CudaCompute
{
    ExternalSurface::ExternalSurface(uint32_t width,
                                     uint32_t height,
                                     uint32_t depth,
                                     uint64_t memory_size,
                                     cudaChannelFormatDesc channel_format,
                                     int32_t memory_handle)
        : _width(width)
        , _height(height)
        , _depth(depth)
    {
        cudaExternalMemoryHandleDesc memory_handle_desc{};
        memory_handle_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
        memory_handle_desc.handle.fd = memory_handle;
        memory_handle_desc.size = memory_size;

        CUDA_CHECKED_CALL(cudaImportExternalMemory(&_external_memory, &memory_handle_desc));

        cudaExternalMemoryMipmappedArrayDesc array_description{};
        array_description.extent.width = width;
        array_description.extent.height = height;
        array_description.extent.depth = depth;
        array_description.formatDesc = channel_format;
        array_description.numLevels = 1;

        CUDA_CHECKED_CALL(cudaExternalMemoryGetMappedMipmappedArray(&_mipmapped_representation, _external_memory, &array_description));
        CUDA_CHECKED_CALL(cudaGetMipmappedArrayLevel(&_array, _mipmapped_representation, 0));

        cudaResourceDesc resource_desc{};
        resource_desc.resType = cudaResourceTypeArray;
        resource_desc.res.array.array = _array;

        CUDA_CHECKED_CALL(cudaCreateSurfaceObject(&_surface, &resource_desc));
    }
    ExternalSurface::~ExternalSurface()
    {
        cudaDestroySurfaceObject(_surface);
        cudaFreeArray(_array);
        cudaFreeMipmappedArray(_mipmapped_representation);
        cudaDestroyExternalMemory(_external_memory);
    }
}