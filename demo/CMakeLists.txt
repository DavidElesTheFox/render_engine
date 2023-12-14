cmake_minimum_required(VERSION 3.18)

make_directory(${GENERATED_INCLUDE_DIR}/demo)
set(DEMO_DATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources)
configure_file(config/demo_resource_config_in.h "${GENERATED_INCLUDE_DIR}/demo/resource_config.h")


set(RENDERING_SYSTEM_DEMO_SRC 
src/main.cpp
src/DemoApplication.cpp
src/DemoApplication.h
src/MultiWindowApplication.h
src/MultiWindowApplication.cpp
src/ApplicationContext.h
src/ApplicationContext.cpp
src/OffScreenTestApplication.h
src/OffScreenTestApplication.cpp
src/DemoSceneBuilder.h
src/DemoSceneBuilder.cpp
src/WindowSetup.h
src/WindowSetup.cpp 
src/VolumeRendererDemo.h
src/VolmeRendererDemo.cpp
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
src/scene/VolumeObject.h
src/scene/VolumeObject.cpp)

set(RENDERING_SYSTEM_DEMO_ASSETS_FILE
src/assets/IMaterial.h
src/assets/AssetDatabase.h
src/assets/NoLitMaterial.h
src/assets/NoLitMaterial.cpp
src/assets/NoLitMaterial.h
src/assets/BillboardMaterial.h
src/assets/BillboardMaterial.cpp
src/assets/CtVolumeMaterial.h 
src/assets/CtVolumeMaterial.cpp)

set(RENDERING_SYSTEM_DEMO_UI_FILE
src/ui/AssetBrowserUi.h
src/ui/AssetBrowserUi.cpp
)

set(RENDERING_SYSTEM_DEMO_CONFIG_FILES
config/demo_resource_config_in.h)

source_group(src/scene FILES ${RENDERING_SYSTEM_DEMO_SCENE_FILE})
source_group(src FILES ${RENDERING_SYSTEM_DEMO_SRC})
source_group(src/assets FILES ${RENDERING_SYSTEM_DEMO_ASSETS_FILE})
source_group(src/ui FILES ${RENDERING_SYSTEM_DEMO_UI_FILE})
source_group(config FILES ${RENDERING_SYSTEM_DEMO_CONFIG_FILES})

add_executable(RenderingSystemDemo ${RENDERING_SYSTEM_DEMO_SRC} ${RENDERING_SYSTEM_DEMO_SCENE_FILE} ${RENDERING_SYSTEM_DEMO_ASSETS_FILE} ${RENDERING_SYSTEM_DEMO_UI_FILE})
set_property(TARGET RenderingSystemDemo PROPERTY CXX_STANDARD 20)
target_include_directories(RenderingSystemDemo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(RenderingSystemDemo PUBLIC RenderEngine)
# STB headers #
add_custom_target(copyStbImageWriteToDemo
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${STB_LIBRARY_INCLUDE_PATH}/stb_image_write.h" ${GENERATED_INCLUDE_DIR}
)
add_dependencies(RenderingSystemDemo copyStbImageWriteToDemo)
########################
# Shaders              #
########################

function(compile_all_shaders SHADER_FILES)

endfunction()


set(RENDERING_SYSTEM_DEMO_SHADER_FILES 
resources/shaders/nolit.vert
resources/shaders/nolit.frag
resources/shaders/billboard.vert
resources/shaders/billboard.frag
resources/shaders/ct_volume.vert
resources/shaders/ct_volume.frag
)

add_custom_target(Shaders 
DEPENDS ${RENDERING_SYSTEM_DEMO_SHADER_FILES}
COMMAND ${CMAKE_COMMAND} -DPARAM_SHADER_FILES="${RENDERING_SYSTEM_DEMO_SHADER_FILES}" -DVulkan_GLSLC_EXECUTABLE="${Vulkan_GLSLC_EXECUTABLE}" -DPARAM_WORKING_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/compile_shaders.cmake"
COMMENT "Compile shader files"
SOURCES ${RENDERING_SYSTEM_DEMO_SHADER_FILES}
)