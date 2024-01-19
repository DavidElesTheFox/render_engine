#pragma once

#include <render_engine/cuda_compute/CudaDevice.h>

#include <future>
#include <memory>
#include <random>
#include <vector>

#include <render_engine/cuda_compute/IComputeCallback.h>

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
            std::unique_ptr<IComputeCallback> on_finished_callback{ nullptr };
        };
        struct ExecutionParameters
        {
            uint32_t thread_count_per_block = 512;
            float algorithms_epsilon = 0.01f;
        };
        explicit DistanceFieldTask(ExecutionParameters execution_parameters = {})
            : _execution_parameters(std::move(execution_parameters))
        {

        }
        DistanceFieldTask(const DistanceFieldTask&) = delete;
        DistanceFieldTask& operator=(const DistanceFieldTask&) = delete;

        DistanceFieldTask(DistanceFieldTask&&) = default;
        DistanceFieldTask& operator=(DistanceFieldTask&&) = default;


        ~DistanceFieldTask() = default;

        void execute(Description&& task_description, CudaDevice* cudaDevice);

        Description clearResult() { return _result.get(); }

        void waitResult() const { _result.wait(); }
        template< class Rep, class Period >
        std::future_status waitResultFor(const std::chrono::duration<Rep, Period>& timeout_duration) const
        {
            return _result.wait_for(timeout_duration);
        }
        bool isReady() const { return waitResultFor(std::chrono::seconds(0)) == std::future_status::ready; }
    private:
        ExecutionParameters _execution_parameters{};
        std::future<Description> _result;
    };
}