cmake_minimum_required(VERSION 3.18)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set(TESTS_SRC
    DistanceFieldTest.cpp 
    CudaDeviceTest.cpp)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(CudaComputeTests ${TESTS_SRC})
target_link_libraries(CudaComputeTests gtest_main CudaCompute)
add_test(NAME cuda_compute_tests COMMAND CudaComputeTests)