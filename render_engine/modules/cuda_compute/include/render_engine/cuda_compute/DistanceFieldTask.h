#pragma once

#include <render_engine/cuda_compute/CudaDevice.h>

#include <future>
#include <memory>
#include <random>
#include <vector>

#include <cuda_runtime_api.h>

namespace RenderEngine::CudaCompute
{
    class DistanceFieldTask
    {
    public:
        struct Description
        {
            cudaSurfaceObject_t input_data;
            cudaSurfaceObject_t output_data;
            uint32_t segmentation_threshold{ 0 };
            uint32_t width{ 0 };
            uint32_t height{ 0 };
            uint32_t depth{ 0 };
            cudaStreamCallback_t on_finished_callback{ nullptr };
            void* on_finish_callback_user_data{ nullptr };
        };
        DistanceFieldTask() = default;
        ~DistanceFieldTask() = default;

        void execute(Description&& task_description, CudaDevice* cudaDevice);

        Description getResult() { return _result.get(); }

        void waitResult() const { _result.wait(); }
        template< class Rep, class Period >
        std::future_status waitResultFor(const std::chrono::duration<Rep, Period>& timeout_duration) const
        {
            return _result.wait_for(timeout_duration);
        }
    private:
        std::future<Description> _result;
    };
}