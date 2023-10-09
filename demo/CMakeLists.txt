

set(RENDERING_SYSTEM_DEMO_SRC main.cpp
DemoApplication.cpp
DemoApplication.h
MultiWindowApplication.h
MultiWindowApplication.cpp
)

add_executable(RenderingSystemDemo ${RENDERING_SYSTEM_DEMO_SRC})
set_property(TARGET RenderingSystemDemo PROPERTY CXX_STANDARD 20)

target_link_libraries(RenderingSystemDemo PUBLIC RenderEngine)