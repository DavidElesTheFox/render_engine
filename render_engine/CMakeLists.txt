cmake_minimum_required(VERSION 3.18)

find_package(Vulkan REQUIRED)


set(EMPTY_SCREEN_IMAGE_PATH "${DATA_DIRECTORY}/empty_screen.bmp")

set(RENDER_ENGINE_HEADER_LOCATION include/render_engine)

configure_file(config/data_config_in.h "${GENERATED_INCLUDE_DIR}/data_config.h")

###############################################################################

#############
# Renderers #
#############
set(RENDER_ENGINE_RENDERS_SRC
    src/renderers/AbstractRenderer.cpp
	src/renderers/ExampleRenderer.cpp
	src/renderers/ForwardRenderer.cpp
	src/renderers/UIRenderer.cpp
    src/renderers/ImageStreamRenderer.cpp
    src/renderers/SingleColorOutputRenderer.cpp
    src/renderers/VolumeRenderer.cpp
    )
set(RENDER_ENGINE_RENDERERS_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/AbstractRenderer.h
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/ExampleRenderer.h
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/ForwardRenderer.h
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/UIRenderer.h
    ${RENDER_ENGINE_HEADER_LOCATION}/renderers/ImageStreamRenderer.h
    ${RENDER_ENGINE_HEADER_LOCATION}/renderers/SingleColorOutputRenderer.h
    ${RENDER_ENGINE_HEADER_LOCATION}/renderers/VolumeRenderer.h
	)
source_group("src\\renderers" FILES ${RENDER_ENGINE_RENDERS_SRC})
source_group("include\\renderers" FILES ${RENDER_ENGINE_RENDERERS_HEADERS})

#############
# resources #
#############

set(RENDER_ENGINE_RESOURCES_SRC
	src/resources/ShaderModule.cpp
	src/resources/Technique.cpp
	src/resources/Buffer.cpp
	src/resources/UniformBinding.cpp
	src/resources/PushConstantsUpdater.cpp
	src/resources/Texture.cpp
    src/resources/RenderTarget.cpp
    src/resources/GpuResourceSet.cpp
    )
set(RENDER_ENGINE_RESOURCES_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/resources/Buffer.h
	${RENDER_ENGINE_HEADER_LOCATION}/resources/ShaderModule.h
	${RENDER_ENGINE_HEADER_LOCATION}/resources/Technique.h
	${RENDER_ENGINE_HEADER_LOCATION}/resources/UniformBinding.h
	${RENDER_ENGINE_HEADER_LOCATION}/resources/PushConstantsUpdater.h
	${RENDER_ENGINE_HEADER_LOCATION}/resources/Texture.h
    ${RENDER_ENGINE_HEADER_LOCATION}/resources/RenderTarget.h
    ${RENDER_ENGINE_HEADER_LOCATION}/resources/GpuResourceSet.h
	)
source_group("src\\resources" FILES ${RENDER_ENGINE_RESOURCES_SRC})
source_group("include\\resources" FILES ${RENDER_ENGINE_RESOURCES_HEADERS})

###################
# synchronization #
###################

set(RENDER_ENGINE_SYNCHRONIZATION_SRC
	src/synchronization/SyncObject.cpp
    src/synchronization/SyncPrimitives.cpp
	src/synchronization/SyncOperations.cpp
	src/synchronization/ResourceStateMachine.cpp

    )
set(RENDER_ENGINE_SYNCHRONIZATION_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/synchronization/SyncObject.h
	${RENDER_ENGINE_HEADER_LOCATION}/synchronization/SyncPrimitives.h
	${RENDER_ENGINE_HEADER_LOCATION}/synchronization/SyncOperations.h
	${RENDER_ENGINE_HEADER_LOCATION}/synchronization/ResourceStateMachine.h
    ${RENDER_ENGINE_HEADER_LOCATION}/synchronization/ResourceStates.h

	)
source_group("src\\synchronization" FILES ${RENDER_ENGINE_SYNCHRONIZATION_SRC})
source_group("include\\synchronization" FILES ${RENDER_ENGINE_SYNCHRONIZATION_HEADERS})


##########
# Assets #
##########

set(RENDER_ENGINE_ASSETS_SRC
	src/assets/Material.cpp
	src/assets/Shader.cpp
	src/assets/Image.cpp
    src/assets/VolumetricObject.cpp
    src/assets/VolumeShader.cpp
    src/assets/VolumeMaterial.cpp
    src/assets/TextureBindingMap.cpp
    src/assets/TextureAssignment.cpp
    src/assets/MaterialInstance.cpp
    src/assets/VolumeMaterialInstance.cpp
    )
set(RENDER_ENGINE_ASSETS_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/assets/Geometry.h
	${RENDER_ENGINE_HEADER_LOCATION}/assets/Material.h
	${RENDER_ENGINE_HEADER_LOCATION}/assets/Mesh.h
	${RENDER_ENGINE_HEADER_LOCATION}/assets/Shader.h
	${RENDER_ENGINE_HEADER_LOCATION}/assets/Image.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/VolumetricObject.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/VolumeShader.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/VolumeMaterial.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/TextureBindingMap.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/TextureAssignment.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/MaterialInstance.h
    ${RENDER_ENGINE_HEADER_LOCATION}/assets/VolumeMaterialInstance.h


	)
source_group("src\\assets" FILES ${RENDER_ENGINE_ASSETS_SRC})
source_group("include\\assets" FILES ${RENDER_ENGINE_ASSETS_HEADERS})

##############
# containers #
##############

set(RENDER_ENGINE_CONTAINERS_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/containers/BackBuffer.h
    ${RENDER_ENGINE_HEADER_LOCATION}/containers/ImageStream.h
    ${RENDER_ENGINE_HEADER_LOCATION}/containers/VariantOverloaded.h
    ${RENDER_ENGINE_HEADER_LOCATION}/containers/Views.h
	)
source_group("include\\containers" FILES ${RENDER_ENGINE_CONTAINERS_HEADERS})

##########
# Window #
##########

set(RENDER_ENGINE_WINDOW_SRC
	src/window/Window.cpp
    src/window/SwapChain.cpp
    src/window/OffScreenWindow.cpp
    src/window/WindowTunnel.cpp
	)
set(RENDER_ENGINE_WINDOW_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/window/Window.h
	${RENDER_ENGINE_HEADER_LOCATION}/window/SwapChain.h
    ${RENDER_ENGINE_HEADER_LOCATION}/window/OffScreenWindow.h
    ${RENDER_ENGINE_HEADER_LOCATION}/window/IWindow.h
    ${RENDER_ENGINE_HEADER_LOCATION}/window/WindowTunnel.h
	)

source_group("src\\window" FILES ${RENDER_ENGINE_WINDOW_SRC})
source_group("include\\window" FILES ${RENDER_ENGINE_WINDOW_HEADERS})

###############
# Rendergraph #
###############

set(RENDER_ENGINE_RENDERGRAPH_SRC
    src/rendergraph/Node.cpp
    src/rendergraph/Link.cpp
    src/rendergraph/Job.cpp
    src/rendergraph/Graph.cpp
    src/rendergraph/GraphVisitor.cpp
    src/rendergraph/TaskflowBuilder.cpp
	)
set(RENDER_ENGINE_RENDERGRAPH_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/rendergraph/Node.h
    ${RENDER_ENGINE_HEADER_LOCATION}/rendergraph/Job.h
    ${RENDER_ENGINE_HEADER_LOCATION}/rendergraph/Link.h
    ${RENDER_ENGINE_HEADER_LOCATION}/rendergraph/Graph.h
    ${RENDER_ENGINE_HEADER_LOCATION}/rendergraph/GraphVisitor.h
    ${RENDER_ENGINE_HEADER_LOCATION}/rendergraph/TaskflowBuilder.h
	)

source_group("src\\rendergraph" FILES ${RENDER_ENGINE_RENDERGRAPH_SRC})
source_group("include\\rendergraph" FILES ${RENDER_ENGINE_RENDERGRAPH_HEADERS})

########
# root #
########

set(RENDER_ENGINE_ROOT_SRC 
		src/RenderEngine.cpp
		src/Device.cpp
		src/RenderContext.cpp
		src/RendererFactory.cpp
		src/GpuResourceManager.cpp
		src/TransferEngine.cpp
		src/CommandPoolFactory.cpp
        src/DeviceLookup.cpp
        src/CommandContext.cpp
        src/LogicalDevice.cpp
        src/DataTransferScheduler.cpp
        src/DataTransferTasks.cpp
        src/Debugger.cpp
        src/ParallelRenderEngine.cpp
        src/QueueSubmitTracker.cpp
)
set(RENDER_ENGINE_ROOT_HEADERS
	${RENDER_ENGINE_HEADER_LOCATION}/RenderEngine.h
	${RENDER_ENGINE_HEADER_LOCATION}/Device.h
	${RENDER_ENGINE_HEADER_LOCATION}/RenderContext.h
	${RENDER_ENGINE_HEADER_LOCATION}/RendererFactory.h
	${RENDER_ENGINE_HEADER_LOCATION}/GpuResourceManager.h
	${RENDER_ENGINE_HEADER_LOCATION}/TransferEngine.h
	${RENDER_ENGINE_HEADER_LOCATION}/CommandPoolFactory.h
    ${RENDER_ENGINE_HEADER_LOCATION}/DeviceLookup.h
    ${RENDER_ENGINE_HEADER_LOCATION}/CommandContext.h
    ${RENDER_ENGINE_HEADER_LOCATION}/LogicalDevice.h
    ${RENDER_ENGINE_HEADER_LOCATION}/DataTransferScheduler.h
    ${RENDER_ENGINE_HEADER_LOCATION}/DataTransferTasks.h
    ${RENDER_ENGINE_HEADER_LOCATION}/Debugger.h
    ${RENDER_ENGINE_HEADER_LOCATION}/ParallelRenderEngine.h
    ${RENDER_ENGINE_HEADER_LOCATION}/QueueSubmitTracker.h
)
source_group("src" FILES ${RENDER_ENGINE_ROOT_SRC})
source_group("include" FILES ${RENDER_ENGINE_ROOT_HEADERS})

###############################################################################

set(RENDER_ENGINE_SRC
    ${RENDER_ENGINE_RENDERS_SRC}
    ${RENDER_ENGINE_RESOURCES_SRC}
    ${RENDER_ENGINE_ASSETS_SRC}
	${RENDER_ENGINE_WINDOW_SRC}
	${RENDER_ENGINE_MEMORY_SRC}
    ${RENDER_ENGINE_SYNCHRONIZATION_SRC}
	${RENDER_ENGINE_ROOT_SRC}
    ${RENDER_ENGINE_RENDERGRAPH_SRC})

set(RENDER_ENGINE_HEADERS 
    ${RENDER_ENGINE_RENDERERS_HEADERS}
    ${RENDER_ENGINE_RESOURCES_HEADERS}
    ${RENDER_ENGINE_ASSETS_HEADERS}
    ${RENDER_ENGINE_CONTAINERS_HEADERS}
	${RENDER_ENGINE_WINDOW_HEADERS}
	${RENDER_ENGINE_MEMORY_HEADERS}
    ${RENDER_ENGINE_SYNCHRONIZATION_HEADERS}
	${RENDER_ENGINE_ROOT_HEADERS} 
    ${RENDER_ENGINE_RENDERGRAPH_HEADERS})

add_library(RenderEngine STATIC ${RENDER_ENGINE_SRC} ${RENDER_ENGINE_HEADERS})
set_property(TARGET RenderEngine PROPERTY CXX_STANDARD 23)

target_compile_options(RenderEngine PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

#####################
# Imgui integration #
#####################

set(RENDER_ENGINE_IMGUI_SOURCES 
"${IMGUI_SRC}/imgui.cpp"
		"${IMGUI_SRC}/imgui.h"
		"${IMGUI_SRC}/imgui_demo.cpp"
		"${IMGUI_SRC}/imgui_draw.cpp"
		"${IMGUI_SRC}/imgui_widgets.cpp"
		"${IMGUI_SRC}/imgui_tables.cpp"
		"${IMGUI_SRC}/backends/imgui_impl_vulkan.h"
		"${IMGUI_SRC}/backends/imgui_impl_vulkan.cpp"
		"${IMGUI_SRC}/backends/imgui_impl_glfw.h"
		"${IMGUI_SRC}/backends/imgui_impl_glfw.cpp")

source_group(imgui_src FILES ${RENDER_ENGINE_IMGUI_SOURCES})
target_sources(RenderEngine 
	PRIVATE
		${RENDER_ENGINE_IMGUI_SOURCES}
)

# disable all warnings from IMGUI
set_source_files_properties(
  ${RENDER_ENGINE_IMGUI_SOURCES}
  PROPERTIES
  COMPILE_FLAGS "/w"
)
########################
# Modules              #
########################

add_subdirectory(modules)

########################
# Library dependencies #
########################

# STB headers #
add_custom_target(copyStbImage
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${STB_LIBRARY_INCLUDE_PATH}/stb_image.h" ${GENERATED_INCLUDE_DIR}
)
add_dependencies(RenderEngine copyStbImage)

# Dependencies #
target_link_libraries(RenderEngine 
	PUBLIC 
        CudaCompute
		glfw
		volk::volk
        glm::glm
        Taskflow)
		
target_compile_definitions(RenderEngine 
	PUBLIC
		GLM_FORCE_RADIANS
	PRIVATE 
		VK_USE_PLATFORM_WIN32_KHR
		VK_NO_PROTOTYPES
		GLFW_INCLUDE_VULKAN
		GLFW_EXPOSE_NATIVE_WIN32)

target_include_directories(RenderEngine 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

# Disable warnings from included external files
target_include_directories(RenderEngine SYSTEM
	PUBLIC
		${GENERATED_INCLUDE_DIR}
		${IMGUI_SRC}
		${RENDERDOC_PATH}

target_link_libraries(RenderEngine 
    SYSTEM
	PUBLIC 
        Taskflow)
)
