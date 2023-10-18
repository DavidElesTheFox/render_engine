

set(RENDERING_SYSTEM_DEMO_SRC 
src/main.cpp
src/DemoApplication.cpp
src/DemoApplication.h
src/MultiWindowApplication.h
src/MultiWindowApplication.cpp
src/ApplicationContext.h
src/ApplicationContext.cpp
)

set(RENDERING_SYSTEM_DEMO_SCENE_FILE
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
src/scene/SceneRenderManager.h
src/scene/SceneRenderManager.cpp
)

set(RENDERING_SYSTEM_DEMO_ASSETS_FILE
src/assets/IMaterial.h
src/assets/AssetDatabase.h
src/assets/NoLitMaterial.h
src/assets/NoLitMaterial.cpp
src/assets/NoLitMaterial.h
)

set(RENDERING_SYSTEM_DEMO_UI_FILE
src/ui/AssetBrowserUi.h
src/ui/AssetBrowserUi.cpp
)

source_group(src/scene FILES ${RENDERING_SYSTEM_DEMO_SCENE_FILE})
source_group(src FILES ${RENDERING_SYSTEM_DEMO_SRC})
source_group(src/assets FILES ${RENDERING_SYSTEM_DEMO_ASSETS_FILE})
source_group(src/ui FILES ${RENDERING_SYSTEM_DEMO_UI_FILE})

add_executable(RenderingSystemDemo ${RENDERING_SYSTEM_DEMO_SRC} ${RENDERING_SYSTEM_DEMO_SCENE_FILE} ${RENDERING_SYSTEM_DEMO_ASSETS_FILE} ${RENDERING_SYSTEM_DEMO_UI_FILE})
set_property(TARGET RenderingSystemDemo PROPERTY CXX_STANDARD 20)
target_include_directories(RenderingSystemDemo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(RenderingSystemDemo PUBLIC RenderEngine)