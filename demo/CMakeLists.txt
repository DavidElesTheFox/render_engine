

set(RENDERING_SYSTEM_DEMO_SRC main.cpp)

add_executable(RenderingSystemDemo ${RENDERING_SYSTEM_DEMO_SRC})

target_link_libraries(RenderingSystemDemo PUBLIC RenderEngine)