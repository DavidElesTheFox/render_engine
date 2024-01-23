#include <render_engine/cuda_compute/CudaDevice.h>

#include <cuda_runtime_api.h>

#include <algorithm>
#include <cassert>

#define CUDA_CHECKED_CALL(exp) {cudaError e = (exp); assert(e == cudaSuccess && #exp);}

namespace RenderEngine::CudaCompute
{
    std::unique_ptr<CudaDevice> CudaDevice::createDeviceForUUID(std::span<uint8_t> device_id, uint32_t stream_count)
    {
        if (device_id.size() != 16)
        {
            throw std::runtime_error("Invalid device id");
        }
        int32_t cuda_device_count{ 0 };
        cudaGetDeviceCount(&cuda_device_count);
        for (int cuda_device_id = 0; cuda_device_id < cuda_device_count; cuda_device_id++)
        {
            cudaDeviceProp deviceProp{};
            cudaGetDeviceProperties(&deviceProp, cuda_device_id);
            if (std::memcmp(device_id.data(), &deviceProp.uuid.bytes, sizeof(uint8_t) * device_id.size()) == 0)
            {
                return std::make_unique<CudaDevice>(cuda_device_id, stream_count);
            }
        }
        return nullptr;
    }
    bool CudaDevice::hasDeviceWithUUIDCudaSupport(std::span<uint8_t> device_id)
    {
        if (device_id.size() != 16)
        {
            throw std::runtime_error("Invalid device id");
        }
        int32_t cuda_device_count{ 0 };
        cudaGetDeviceCount(&cuda_device_count);
        for (int cuda_device_id = 0; cuda_device_id < cuda_device_count; cuda_device_id++)
        {
            cudaDeviceProp deviceProp{};
            cudaGetDeviceProperties(&deviceProp, cuda_device_id);
            if (std::memcmp(device_id.data(), &deviceProp.uuid.bytes, sizeof(uint8_t) * device_id.size()) == 0)
            {
                return true;
            }
        }
        return false;
    }
    CudaDevice::CudaDevice(int32_t id, uint32_t stream_count)
        : _device_id{ id }
        , _stream_queue(stream_count, StreamSlot{})
    {
        cudaDeviceProp p;
        cudaGetDeviceProperties(&p, _device_id);
        std::copy(p.uuid.bytes, p.uuid.bytes + sizeof(p.uuid), _device_uuid.begin());

        for (uint32_t i = 0; i < stream_count; ++i)
        {
            CUDA_CHECKED_CALL(cudaStreamCreate(&_stream_queue[i].stream));
        }
    }

    CudaDevice::Stream CudaDevice::getAvailableStream()
    {
        std::unique_lock lock(_stream_mutex);
        auto find_available_stream = [&]
            {
                return std::find_if(_stream_queue.begin(), _stream_queue.end(),
                                    [](const auto& stream_slot) { return stream_slot.in_use == false; });
            };
        auto it = find_available_stream();
        if (it == _stream_queue.end())
        {
            _stream_available.wait(lock, [&] { return find_available_stream() != _stream_queue.end(); });
            it = std::find_if(_stream_queue.begin(), _stream_queue.end(),
                              [](const auto& stream_slot) { return stream_slot.in_use == false; });
            assert(it != _stream_queue.end());
        }
        it->in_use = true;
        return { *this, it->stream };
    }

    bool CudaDevice::hasAvailableStream() const
    {
        std::unique_lock lock(_stream_mutex);
        return std::any_of(_stream_queue.begin(), _stream_queue.end(),
                           [](const auto& stream_slot) { return stream_slot.in_use == false; });
    }

    void CudaDevice::releaseStream(Stream& stream) noexcept
    {
        {
            std::unique_lock lock(_stream_mutex);
            auto it = std::find_if(_stream_queue.begin(), _stream_queue.end(),
                                   [&](const auto& stream_slot) { return stream_slot.stream == stream.getCudaStream(); });

            it->in_use = false;
        }
        _stream_available.notify_one();
    }


}