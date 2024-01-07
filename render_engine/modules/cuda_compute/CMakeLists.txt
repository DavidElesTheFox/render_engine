cmake_minimum_required(VERSION 3.18)

#include(CheckLanguague)

#checkLanguague(CUDA)
enable_language(CUDA)

set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

include(cmake/CudaConvertFlags.cmake)

set(CUDA_COMPUTE_HEADERS
   include/render_engine/cuda_compute/DistanceFieldTask.h
   include/render_engine/cuda_compute/DistanceFieldKernel.h
   include/render_engine/cuda_compute/CudaDevice.h "include/render_engine/cuda_compute/IComputeCallback.h")

set(CUDA_COMPUTE_SRC
    src/DistanceFieldTask.cpp
    src/DistanceFieldKernel.cu 
    src/CudaDevice.cpp "src/CudaStreamCallback.h" "src/CudaStreamCallback.cpp")


add_library(CudaCompute STATIC ${CUDA_COMPUTE_SRC} ${CUDA_COMPUTE_HEADERS})
set_target_properties(CudaCompute PROPERTIES CUDA_ARCHITECTURES "86")

CUDA_CONVERT_FLAGS(CudaCompute)

set_target_properties(CudaCompute PROPERTIES
                    CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(CudaCompute 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/

	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
                # for vs intellisense
        "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.3\\include"
)
target_compile_options(CudaCompute PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-g -G>)


add_subdirectory(tests)