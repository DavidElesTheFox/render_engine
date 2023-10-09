cmake_minimum_required(VERSION 3.18)

Project(RENDERING_SYSTEM)
option(ENABLE_RENDERDOC "When it is enabled renderdoc is tryed to be loaded from the RENDERDOC_PATH" OFF)

set(GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")
file(MAKE_DIRECTORY ${GENERATED_INCLUDE_DIR})
set(DATA_DIRECTORY ${CMAKE_SOURCE_DIR}/data_dir)
set(IMGUI_SRC "${CMAKE_SOURCE_DIR}/3rd_party/imgui")
set(RENDERDOC_PATH "C:/Program Files/RenderDoc" CACHE STRING "Renderdoc path to api and include files")


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/3rd_party/glfw/cmake" "${CMAKE_SOURCE_DIR}/3rd_party/eigen/cmake")
set(VOLK_STATIC_DEFINES VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR)

add_subdirectory(3rd_party/glfw-src)
add_subdirectory(3rd_party/volk-src)
add_subdirectory(3rd_party/glm-src)

add_subdirectory(render_engine)
add_subdirectory(demo)
