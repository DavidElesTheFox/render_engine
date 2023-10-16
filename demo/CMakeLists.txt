

set(RENDERING_SYSTEM_DEMO_SRC 
src/main.cpp
src/DemoApplication.cpp
src/DemoApplication.h
src/MultiWindowApplication.h
src/MultiWindowApplication.cpp
)

set(RENDERING_SYSTEM_SCENE_HEADERS
src/scene/Camera.h
src/scene/Camera.cpp
src/scene/Scene.h
src/scene/Scene.cpp
src/scene/SceneNode.h
src/scene/SceneNode.cpp
src/scene/Transformation.h
src/scene/SceneObject.h
src/scene/MeshObject.h
src/scene/MeshObject.cpp
src/scene/SceneNodeLookup.h
src/scene/SceneNodeLookup.cpp
)

source_group(src/scene FILES ${RENDERING_SYSTEM_SCENE_HEADERS})
source_group(src FILES ${RENDERING_SYSTEM_DEMO_SRC})

add_executable(RenderingSystemDemo ${RENDERING_SYSTEM_DEMO_SRC} ${RENDERING_SYSTEM_SCENE_HEADERS})
set_property(TARGET RenderingSystemDemo PROPERTY CXX_STANDARD 20)
target_include_directories(RenderingSystemDemo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(RenderingSystemDemo PUBLIC RenderEngine)