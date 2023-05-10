cmake_minimum_required(VERSION 3.18)

Project(RENDERING_SYSTEM)

set(GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")
file(MAKE_DIRECTORY ${GENERATED_INCLUDE_DIR})
set(DATA_DIRECTORY ${CMAKE_SOURCE_DIR}/data_dir)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/3rd_party/glfw/cmake", "${CMAKE_SOURCE_DIR}/3rd_party/eigen/cmake")
#add_subdirectory(3rd_party/glfw-src)
add_subdirectory(3rd_party/sdl-src)
add_subdirectory(render_engine)
add_subdirectory(demo)