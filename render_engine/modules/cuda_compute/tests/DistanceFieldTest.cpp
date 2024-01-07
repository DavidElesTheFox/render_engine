#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <render_engine/cuda_compute/DistanceFieldKernel.h>
#include <render_engine/cuda_compute/DistanceFieldTask.h>

#include <format>

namespace RenderEngine::CudaCompute::Tests
{
    namespace
    {
        class TaskFinishedCallback : public IComputeCallback
        {
        public:
            void call() override
            {
                {
                    std::lock_guard lock{ _task_mutex };
                    _task_finished = true;
                }
                _task_condition.notify_one();
            }

            template<typename T>
            void wait_for(const T& timeout)
            {
                std::unique_lock lock(_task_mutex);
                _task_condition.wait_for(lock, timeout, [&] { return _task_finished; });
            }

            bool isTaskFinished() const
            {
                std::lock_guard lock(_task_mutex);
                return _task_finished;
            }
        private:
            bool _task_finished = false;
            mutable std::mutex _task_mutex;
            std::condition_variable _task_condition;
        };
    }


    class DistanceFieldTest : public testing::Test
    {
    public:
        struct SurfaceWrapper
        {
            ~SurfaceWrapper()
            {
                cudaDestroySurfaceObject(surface);
                cudaFreeArray(memory);
            }
            cudaSurfaceObject_t surface{};
            cudaArray_t memory{};
        };

    protected:

        template<typename T>
        cudaMemcpy3DParms createCopyParams(std::vector<T>& host_data, cudaArray_t device_data, uint32_t width, uint32_t height, uint32_t depth, cudaMemcpyKind memcopy_kind)
        {
            const cudaPitchedPtr pitched_ptr = make_cudaPitchedPtr(host_data.data(), width * sizeof(T), width, height);

            cudaMemcpy3DParms copy_params{};
            switch (memcopy_kind)
            {
                case cudaMemcpyDeviceToHost:
                    copy_params.srcArray = device_data;
                    copy_params.dstPtr = pitched_ptr;
                    break;
                case cudaMemcpyHostToDevice:
                    copy_params.srcPtr = pitched_ptr;
                    copy_params.dstArray = device_data;
                    break;
                default:
                    throw std::runtime_error("Unhandled memcpy kind");

            }

            copy_params.extent = { width, height, depth };
            copy_params.kind = memcopy_kind;
            return copy_params;
        }

        SurfaceWrapper createInputSurface(const std::vector<uint32_t>& data, uint32_t width, uint32_t height, uint32_t depth)
        {
            cudaArray_t memory = DistanceFieldKernel::allocateInputMemory(width, height, depth);
            if (depth > 1)
            {
                // const cast is necessary anyways, due to make_cudaPitchedPtr
                cudaMemcpy3DParms copy_params = createCopyParams(const_cast<std::vector<uint32_t>&>(data), memory, width, height, depth, cudaMemcpyHostToDevice);

                if (auto error = cudaMemcpy3D(&copy_params); error != cudaSuccess)
                {
                    throw std::runtime_error("Cannot copy cuda memory: " + std::string{ cudaGetErrorString(error) });
                };
                cudaDeviceSynchronize();
            }
            else
            {
                cudaMemcpyToArray(memory, 0, 0, data.data(), data.size() * sizeof(uint32_t), cudaMemcpyHostToDevice);
            }
            cudaResourceDesc resource_desc{};
            resource_desc.res.array.array = memory;
            resource_desc.resType = cudaResourceTypeArray;

            cudaSurfaceObject_t result{};

            if (cudaError_t ok = cudaCreateSurfaceObject(&result, &resource_desc); ok != cudaSuccess)
            {
                throw std::runtime_error("Cannot create input surface: " + std::to_string(ok));
            }

            return { result, memory };
        }
        SurfaceWrapper createOutputSurface(uint32_t width, uint32_t height, uint32_t depth)
        {
            cudaArray_t memory = DistanceFieldKernel::allocateOutputMemory(width, height, depth);
            cudaResourceDesc resource_desc{};
            resource_desc.res.array.array = memory;
            resource_desc.resType = cudaResourceTypeArray;

            cudaSurfaceObject_t result{};

            if (cudaError_t error = cudaCreateSurfaceObject(&result, &resource_desc); error != cudaSuccess)
            {
                throw std::runtime_error("Cannot create input surface: " + std::string{ cudaGetErrorString(error) });
            }

            return { result, memory };
        }


        void readBack(std::vector<float>& result_data, cudaArray_t d_memory, uint32_t width, uint32_t height, uint32_t depth)
        {
            if (depth > 0)
            {
                cudaMemcpy3DParms copy_params = createCopyParams(result_data, d_memory, width, height, depth, cudaMemcpyDeviceToHost);


                if (auto error = cudaMemcpy3D(&copy_params); error != cudaSuccess)
                {
                    FAIL() << "Data couldn't read back " << cudaGetErrorString(error);
                }
            }
            else
            {
                cudaMemcpyFromArray(result_data.data(), d_memory, 0, 0, sizeof(float) * width * height * depth, cudaMemcpyDeviceToHost);
            }
        }

        uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b)
        {
            return r | g << 8 | b << 16;
        }

        static bool epsilonEquals(float a, float b, float epsilon = 0.01f)
        {
            return std::abs(a - b) < epsilon;
        }
        bool isVerbose() const { return _verbose; }
        void setVerbose(bool value) { _verbose = value; }
    private:
        bool _verbose{ false };
    };

    TEST_F(DistanceFieldTest, distance_from_center)
    {
        const uint32_t image_width = 4;
        const uint32_t image_height = 4;
        const uint32_t image_depth = 1;
        const std::vector<uint32_t> data =
        {
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
        };
        std::vector<float> result_data(image_width * image_height * image_depth, -1.0f);


        auto input = createInputSurface(data, image_width, image_height, image_depth);
        auto result = createOutputSurface(image_width, image_height, image_depth);

        DistanceFieldKernel::KernelParameters kernel_parameters;
        kernel_parameters.block_size = dim3{ 2, 2, 1 };
        kernel_parameters.grid_size = dim3{ 2, 2, 1 };

        DistanceFieldKernel kernel{ kernel_parameters };

        kernel.run(input.surface, result.surface, 2, 0.0f);


        readBack(result_data, result.memory, image_width, image_height, image_depth);

        const std::vector<float> expected_distances =
        {
            0.71f, 0.56f, 0.50f, 0.56f,
            0.56f, 0.35f, 0.25f, 0.35f,
            0.50f, 0.25f, 0.0f, 0.25f,
            0.56f, 0.35f, 0.25f, 0.35f
        };

        for (uint32_t i = 0; i < image_width; ++i)
        {
            for (uint32_t j = 0; j < image_height; ++j)
            {
                EXPECT_TRUE(epsilonEquals(expected_distances[j * image_width + i], result_data[j * image_width + i]));
                if (isVerbose())
                {
                    std::cout << std::format("{0:.2f}f, ", result_data[j * image_width + i]);
                }
            }
            if (isVerbose())
            {
                std::cout << std::endl;
            }
        }
    }

    TEST_F(DistanceFieldTest, distance_from_center_overflow)
    {
        const uint32_t image_width = 5;
        const uint32_t image_height = 5;
        const uint32_t image_depth = 1;
        const std::vector<uint32_t> data =
        {
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),makeColor(0, 0, 0)
        };
        std::vector<float> result_data(image_width * image_height * image_depth, -1.0f);


        auto input = createInputSurface(data, image_width, image_height, image_depth);
        auto result = createOutputSurface(image_width, image_height, image_depth);

        DistanceFieldKernel::KernelParameters kernel_parameters;
        kernel_parameters.block_size = dim3{ 3, 3, 1 };
        kernel_parameters.grid_size = dim3{ 2, 2, 1 };

        DistanceFieldKernel kernel{ kernel_parameters };

        kernel.run(input.surface, result.surface, 2, 0.0f);

        readBack(result_data, result.memory, image_width, image_height, image_depth);

        const std::vector<float> expected_distances =
        {
            0.47f, 0.37f, 0.33f, 0.37f, 0.47f,
            0.37f, 0.24f, 0.17f, 0.24f, 0.37f,
            0.33f, 0.17f, 0.00f, 0.17f, 0.33f,
            0.37f, 0.24f, 0.17f, 0.24f, 0.37f,
            0.47f, 0.37f, 0.33f, 0.37f, 0.47f
        };

        for (uint32_t i = 0; i < image_width; ++i)
        {
            for (uint32_t j = 0; j < image_height; ++j)
            {
                EXPECT_TRUE(epsilonEquals(expected_distances[j * image_width + i], result_data[j * image_width + i]));
                if (isVerbose())
                {
                    std::cout << std::format("{0:.2f}f, ", result_data[j * image_width + i]);
                }
            }
            if (isVerbose())
            {
                std::cout << std::endl;
            }
        }
    }

    TEST_F(DistanceFieldTest, distance_from_circle)
    {
        const uint32_t image_width = 6;
        const uint32_t image_height = 6;
        const uint32_t image_depth = 1;
        const std::vector<uint32_t> data =
        {
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(3, 3, 3), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(3, 3, 3), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0)
        };
        std::vector<float> result_data(image_width * image_height * image_depth, -1.0f);


        auto input = createInputSurface(data, image_width, image_height, image_depth);
        auto result = createOutputSurface(image_width, image_height, image_depth);

        DistanceFieldKernel::KernelParameters kernel_parameters;
        kernel_parameters.block_size = dim3{ 3, 3, 1 };
        kernel_parameters.grid_size = dim3{ 2, 2, 1 };

        DistanceFieldKernel kernel{ kernel_parameters };

        kernel.run(input.surface, result.surface, 2, 0.0f);

        readBack(result_data, result.memory, image_width, image_height, image_depth);

        const std::vector<float> expected_distances =
        {
            0.37f, 0.24f, 0.17f, 0.17f, 0.24f, 0.37f,
            0.24f, 0.17f, 0.00f, 0.00f, 0.17f, 0.24f,
            0.17f, 0.00f, 0.17f, 0.17f, 0.00f, 0.17f,
            0.17f, 0.00f, 0.17f, 0.17f, 0.00f, 0.17f,
            0.24f, 0.17f, 0.00f, 0.00f, 0.17f, 0.24f,
            0.37f, 0.24f, 0.17f, 0.17f, 0.24f, 0.37f
        };

        for (uint32_t i = 0; i < image_width; ++i)
        {
            for (uint32_t j = 0; j < image_height; ++j)
            {
                //EXPECT_TRUE(epsilonEquals(expected_distances[j * image_width + i], result_data[j * image_width + i]));
                if (isVerbose())
                {
                    std::cout << std::format("{0:.2f}f, ", result_data[j * image_width + i]);
                }
            }
            if (isVerbose())
            {
                std::cout << std::endl;
            }
        }
    }

    TEST_F(DistanceFieldTest, distance_from_center_3d)
    {
        const uint32_t image_width = 4;
        const uint32_t image_height = 4;
        const uint32_t image_depth = 4;
        const std::vector<uint32_t> data =
        {
            // depth = 0
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),

            // depth = 1
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),

            // depth = 2
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),

            // depth = 3
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0)
        };
        std::vector<float> result_data(image_width * image_height * image_depth, -1.0f);


        auto input = createInputSurface(data, image_width, image_height, image_depth);
        auto result = createOutputSurface(image_width, image_height, image_depth);

        DistanceFieldKernel::KernelParameters kernel_parameters;
        kernel_parameters.block_size = dim3{ 2, 2, 2 };
        kernel_parameters.grid_size = dim3{ 2, 2, 2 };

        DistanceFieldKernel kernel{ kernel_parameters };
        kernel.run(input.surface, result.surface, 2, 0.0f);

        readBack(result_data, result.memory, image_width, image_height, image_depth);


        const std::vector<float> expected_distances =
        {
            // depth = 0
            0.75f, 0.61f, 0.56f, 0.61f,
            0.61f, 0.43f, 0.35f, 0.43f,
            0.56f, 0.35f, 0.25f, 0.35f,
            0.61f, 0.43f, 0.35f, 0.43f,

            // depth = 1
            0.71f, 0.56f, 0.50f, 0.56f,
            0.56f, 0.35f, 0.25f, 0.35f,
            0.50f, 0.25f, 0.00f, 0.25f,
            0.56f, 0.35f, 0.25f, 0.35f,

            // depth = 2
            0.71f, 0.56f, 0.50f, 0.56f,
            0.56f, 0.35f, 0.25f, 0.35f,
            0.50f, 0.25f, 0.00f, 0.25f,
            0.56f, 0.35f, 0.25f, 0.35f,

            // depth = 3
            0.71f, 0.56f, 0.50f, 0.56f,
            0.56f, 0.35f, 0.25f, 0.35f,
            0.50f, 0.25f, 0.00f, 0.25f,
            0.56f, 0.35f, 0.25f, 0.35f
        };

        for (uint32_t k = 0; k < image_depth; ++k)
        {
            if (isVerbose())
            {
                std::cout << "depth = " << k << std::endl;
            }
            for (uint32_t i = 0; i < image_width; ++i)
            {
                for (uint32_t j = 0; j < image_height; ++j)
                {
                    EXPECT_TRUE(epsilonEquals(expected_distances[j * image_width + i], result_data[j * image_width + i]));
                    if (isVerbose())
                    {
                        std::cout << std::format("{0:.2f}f, ", result_data[k * (image_height * image_width) + j * image_width + i]);
                    }
                }
                if (isVerbose())
                {
                    std::cout << std::endl;
                }
            }
            if (isVerbose())
            {
                std::cout << std::endl;
            }
        }
    }

    TEST_F(DistanceFieldTest, distance_from_center_with_task)
    {
        const uint32_t image_width = 4;
        const uint32_t image_height = 4;
        const uint32_t image_depth = 1;
        const std::vector<uint32_t> data =
        {
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(3, 3, 3), makeColor(0, 0, 0),
            makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0), makeColor(0, 0, 0),
        };
        std::vector<float> result_data(image_width * image_height * image_depth, -1.0f);


        auto input = createInputSurface(data, image_width, image_height, image_depth);
        auto result = createOutputSurface(image_width, image_height, image_depth);

        CudaDevice device(0, 1);
        // Currently the 'image size' is determined by the kernel parameters that calculated from the thread count. 
        // If there are too many threads the algorithm will give less precise result due to the integer projection [0,1] -> [0, 1023]
        DistanceFieldTask task(DistanceFieldTask::ExecutionParameters{ .thread_count_per_block = 6 });

        // This pointer will be valid until the result is not fetched form the task or clearResult is not called
        TaskFinishedCallback* finish_callback_reference{ nullptr };
        {
            auto finish_callback = std::make_unique<TaskFinishedCallback>();
            DistanceFieldTask::Description task_description;
            task_description.width = image_width;
            task_description.height = image_height;
            task_description.depth = image_depth;
            task_description.input_data = input.surface;
            task_description.output_data = result.surface;
            task_description.segmentation_threshold = 2;

            finish_callback_reference = finish_callback.get();
            task_description.on_finished_callback = std::move(finish_callback);

            task.execute(std::move(task_description), &device);
        }
        {
            using namespace std::chrono_literals;
            finish_callback_reference->wait_for(30s);
            ASSERT_TRUE(finish_callback_reference->isTaskFinished()) << "Task time out error";
        }
        readBack(result_data, result.memory, image_width, image_height, image_depth);

        const std::vector<float> expected_distances =
        {
            0.71f, 0.56f, 0.50f, 0.56f,
            0.56f, 0.35f, 0.25f, 0.35f,
            0.50f, 0.25f, 0.0f, 0.25f,
            0.56f, 0.35f, 0.25f, 0.35f
        };

        for (uint32_t i = 0; i < image_width; ++i)
        {
            for (uint32_t j = 0; j < image_height; ++j)
            {
                if (isVerbose())
                {
                    std::cout << std::format("{0:.2f}f, ", result_data[j * image_width + i]);
                }
                else
                {
                    EXPECT_TRUE(epsilonEquals(expected_distances[j * image_width + i], result_data[j * image_width + i]));
                }
            }
            if (isVerbose())
            {
                std::cout << std::endl;
            }
        }
    }
}
