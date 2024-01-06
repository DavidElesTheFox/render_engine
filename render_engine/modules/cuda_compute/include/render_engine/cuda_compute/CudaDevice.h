#pragma once

#include <cuda_runtime_api.h>

#include <array>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <vector>

namespace RenderEngine::CudaCompute
{
    class CudaDevice
    {
    public:
        class Stream
        {
        public:
            friend class CudaDevice;
            ~Stream()
            {
                _device.releaseStream(*this);
            }
            cudaStream_t getCudaStream() const { return _stream; }
        private:
            Stream(CudaDevice& device, cudaStream_t stream)
                : _device(device)
                , _stream(stream)
            {}
            cudaStream_t _stream;
            CudaDevice& _device;
        };
        CudaDevice(int32_t id, uint32_t stream_count);

        Stream getAvailableStream();
        // TODO implement a read lock mechanism. Currently it is used only for test.
        bool hasAvailableStream() const;
        std::span<const uint8_t> getDeviceUuid() const { return { _device_uuid }; }
        void releaseStream(Stream& stream) noexcept;
    private:
        struct StreamSlot
        {
            cudaStream_t stream{ nullptr };
            bool in_use{ false };
        };
        std::array<uint8_t, 16> _device_uuid;
        std::vector<StreamSlot> _stream_queue;
        std::condition_variable _stream_available;
        mutable std::mutex _stream_mutex;
        int32_t _device_id;
    };
}