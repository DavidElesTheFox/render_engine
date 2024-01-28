#include <render_engine/cuda_compute/DistanceFieldTask.h>

#include <render_engine/cuda_compute/DistanceFieldKernel.h>

#include <cstdint>
#include <future>
#include <iostream>

#include <CudaStreamCallback.h>

namespace RenderEngine::CudaCompute
{
    namespace
    {

        DistanceFieldTask::Description runTask(DistanceFieldTask::Description&& task_description,
                                               CudaDevice* cudaDevice,
                                               DistanceFieldTask::ExecutionParameters execution_params)
        {
            DistanceFieldKernel::KernelParameters kernel_params;

            kernel_params.block_size = dim3(execution_params.thread_count_per_block / 3, execution_params.thread_count_per_block / 3, execution_params.thread_count_per_block / 3);
            kernel_params.grid_size.x = std::ceil(task_description.width / static_cast<float>(kernel_params.block_size.x));
            kernel_params.grid_size.y = std::ceil(task_description.height / static_cast<float>(kernel_params.block_size.y));
            kernel_params.grid_size.z = std::ceil(task_description.depth / static_cast<float>(kernel_params.block_size.z));

            auto stream = cudaDevice->getAvailableStream();
            DistanceFieldKernel kernel(std::move(kernel_params));

            kernel.run(task_description.input_data, task_description.output_data, task_description.segmentation_threshold, task_description.epsilon_distance);

            if (task_description.on_finished_callback != nullptr)
            {
                constexpr auto flags = 0;
                cudaStreamAddCallback(stream.getCudaStream(), cudaStreamCallbackImpl, task_description.on_finished_callback.get(), flags);
            }
            return task_description;
        }
    }

    void DistanceFieldTask::execute(Description&& task_description, CudaDevice* cudaDevice)
    {
        _result = std::async(std::launch::async, runTask, std::move(task_description), cudaDevice, _execution_parameters);
    }


}