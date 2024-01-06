#include <render_engine/cuda_compute/DistanceFieldTask.h>

#include <render_engine/cuda_compute/DistanceFieldKernel.h>

#include <cstdint>
#include <future>
#include <iostream>

namespace RenderEngine::CudaCompute
{
    namespace
    {
        struct ExecutionParameters
        {
            uint32_t thread_count_per_block = 6;
            float algorithms_epsilon = 0.01f;
        };
        DistanceFieldTask::Description runTask(DistanceFieldTask::Description&& task_description,
                                               CudaDevice* cudaDevice,
                                               ExecutionParameters execution_params)
        {
            DistanceFieldKernel::KernelParameters kernel_params;

            kernel_params.block_size = dim3(execution_params.thread_count_per_block / 3);
            kernel_params.grid_size.x = std::ceil(task_description.width / static_cast<float>(kernel_params.block_size.x));
            kernel_params.grid_size.y = std::ceil(task_description.width / static_cast<float>(kernel_params.block_size.y));
            kernel_params.grid_size.z = std::ceil(task_description.width / static_cast<float>(kernel_params.block_size.z));

            auto stream = cudaDevice->getAvailableStream();
            DistanceFieldKernel kernel(std::move(kernel_params));

            kernel.run(task_description.input_data, task_description.output_data, task_description.segmentation_threshold, 0.0f);

            if (task_description.on_finished_callback != nullptr)
            {
                constexpr auto flags = 0;
                cudaStreamAddCallback(stream.getCudaStream(), task_description.on_finished_callback, task_description.on_finish_callback_user_data, flags);
            }
            return task_description;
        }
    }

    void DistanceFieldTask::execute(Description&& task_description, CudaDevice* cudaDevice)
    {
        _result = std::async(std::launch::async, runTask, std::move(task_description), cudaDevice, ExecutionParameters{});
    }


}