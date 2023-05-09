cmake_minimum_required(VERSION 3.18)

find_package(Vulkan REQUIRED)

set(RENDER_ENGINE_HEADER_LOCATION include/render_engine)

set(RENDER_ENGINE_SRC src/RenderEngine.cpp)

set(RENDER_ENGINE_HEADERS ${RENDER_ENGINE_HEADER_LOCATION}/RenderEngine.h)

add_library(RenderEngine STATIC ${RENDER_ENGINE_SRC} ${RENDER_ENGINE_HEADERS})

target_link_libraries(RenderEngine PUBLIC glfw)

target_include_directories(RenderEngine PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)