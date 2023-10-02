cmake_minimum_required(VERSION 3.18)

find_package(Vulkan REQUIRED)

set(EMPTY_SCREEN_IMAGE_PATH "${DATA_DIRECTORY}/empty_screen.bmp")

set(RENDER_ENGINE_HEADER_LOCATION include/render_engine)

configure_file(config/data_config_in.h "${GENERATED_INCLUDE_DIR}/data_config.h")

set(RENDER_ENGINE_RENDERS_SRC
    src/renderers/AbstractRenderer.cpp
	src/renderers/ExampleRenderer.cpp
	src/renderers/UIRenderer.cpp
    )
set(RENDER_ENGINE_RENDERERS_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/AbstractRenderer.h
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/ExampleRenderer.h
	${RENDER_ENGINE_HEADER_LOCATION}/renderers/UIRenderer.h
	)
source_group("src\\renderers" FILES ${RENDER_ENGINE_RENDERS_SRC})
source_group("include\\renderers" FILES ${RENDER_ENGINE_RENDERERS_HEADERS})
	
set(RENDER_ENGINE_MEMORY_SRC
    src/memory/Buffer.cpp
	)
set(RENDER_ENGINE_MEMORY_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/memory/Buffer.h
	)
source_group("src\\memory" FILES ${RENDER_ENGINE_MEMORY_SRC})
source_group("include\\memory" FILES ${RENDER_ENGINE_MEMORY_HEADERS})

set(RENDER_ENGINE_WINDOW_SRC
	src/window/Window.cpp
    src/window/SwapChain.cpp
	)
set(RENDER_ENGINE_WINDOW_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/window/Window.h
	${RENDER_ENGINE_HEADER_LOCATION}/window/SwapChain.h
	)

source_group("src\\window" FILES ${RENDER_ENGINE_WINDOW_SRC})
source_group("include\\window" FILES ${RENDER_ENGINE_WINDOW_HEADERS})

set(RENDER_ENGINE_ROOT_SRC 
		src/RenderEngine.cpp
		src/RenderContext.cpp
		src/RendererFactory.cpp
)
set(RENDER_ENGINE_ROOT_HEADERS
	${RENDER_ENGINE_HEADER_LOCATION}/RenderEngine.h
	${RENDER_ENGINE_HEADER_LOCATION}/RenderContext.h
	${RENDER_ENGINE_HEADER_LOCATION}/RendererFactory.h
)
source_group("src" FILES ${RENDER_ENGINE_ROOT_SRC})
source_group("include" FILES ${RENDER_ENGINE_ROOT_HEADERS})

set(RENDER_ENGINE_SRC
    ${RENDER_ENGINE_RENDERS_SRC}
	${RENDER_ENGINE_WINDOW_SRC}
	${RENDER_ENGINE_MEMORY_SRC}
	${RENDER_ENGINE_ROOT_SRC}
	)

set(RENDER_ENGINE_HEADERS 
    ${RENDER_ENGINE_RENDERERS_HEADERS}
	${RENDER_ENGINE_WINDOW_HEADERS}
	${RENDER_ENGINE_MEMORY_HEADERS}
	${RENDER_ENGINE_ROOT_HEADERS}
	)

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

add_library(RenderEngine STATIC ${RENDER_ENGINE_SRC} ${RENDER_ENGINE_HEADERS})
set_property(TARGET RenderEngine PROPERTY CXX_STANDARD 20)

target_sources(RenderEngine 
	PRIVATE
		${RENDER_ENGINE_IMGUI_SOURCES}
)

target_link_libraries(RenderEngine 
	PUBLIC 
		glfw
		volk::volk)
		
target_compile_definitions(RenderEngine 
	PRIVATE 
		VK_USE_PLATFORM_WIN32_KHR
		VK_NO_PROTOTYPES
		GLFW_INCLUDE_VULKAN
		GLFW_EXPOSE_NATIVE_WIN32)

target_include_directories(RenderEngine 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
		${GENERATED_INCLUDE_DIR}
		${IMGUI_SRC}
		${RENDERDOC_PATH})
