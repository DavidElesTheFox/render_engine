#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <render_engine/cuda_compute/CudaDevice.h>

#include <format>
#include <future>

namespace RenderEngine::CudaCompute::Tests
{

    TEST(CudaDeviceTest, parallel_execution)
    {
        using namespace std::chrono_literals;
        enum class TestPhase
        {
            Idle,
            StartThread2,
            StopThread1
        };
        constexpr auto device_id = 0;
        constexpr auto stream_count = 1;
        CudaDevice device(device_id, stream_count);
        std::atomic_uint32_t error_id{ 0 };

        std::mutex phase_mutex;
        TestPhase test_phase = TestPhase::Idle;
        std::condition_variable test_phase_condition;

        std::thread thread_1(
            [&]
            {
                auto stream = device.getAvailableStream();
                {
                    std::lock_guard lock(phase_mutex);
                    test_phase = TestPhase::StartThread2;
                }
                test_phase_condition.notify_one();

                {
                    std::unique_lock lock(phase_mutex);
                    const bool ok = test_phase_condition.wait_for(lock, 10s, [&] { return test_phase == TestPhase::StopThread1; });
                }
            }
        );

        std::thread thread_2(
            [&]
            {
                {
                    std::unique_lock lock(phase_mutex);
                    const bool ok = test_phase_condition.wait_for(lock, 10s, [&] { return test_phase == TestPhase::StartThread2; });
                    if (ok == false)
                    {
                        error_id = 1; // thread 1 is not started within the timeout time
                        return;
                    }
                }
                if (device.hasAvailableStream())
                {
                    error_id = 2; // The only one stream should be still in use in thread_1
                    return;
                }
                {
                    std::lock_guard lock(phase_mutex);
                    test_phase = TestPhase::StopThread1;
                }
                test_phase_condition.notify_one();
                auto stream = device.getAvailableStream();
            }
        );

        auto join_1 = std::async(std::launch::async, &std::thread::join, &thread_1);
        auto join_2 = std::async(std::launch::async, &std::thread::join, &thread_2);
        if (join_1.wait_for(5s) == std::future_status::timeout)
        {
            thread_1.detach();
            thread_2.detach();
            FAIL() << "Thread one is still running";
        }
        if (join_2.wait_for(5s) == std::future_status::timeout)
        {
            thread_2.detach();
            FAIL() << "Thread two is still running";
        }
        ASSERT_EQ(error_id, 0) << "Expect no error during the execution";
    }
}