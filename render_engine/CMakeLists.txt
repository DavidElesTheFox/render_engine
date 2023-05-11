cmake_minimum_required(VERSION 3.18)

find_package(Vulkan REQUIRED)

set(EMPTY_SCREEN_IMAGE_PATH "${DATA_DIRECTORY}/empty_screen.bmp")

set(RENDER_ENGINE_HEADER_LOCATION include/render_engine)

configure_file(config/data_config_in.h ${GENERATED_INCLUDE_DIR}/data_config.h)

set(RENDER_ENGINE_SRC 
	src/RenderEngine.cpp
	src/RenderContext.cpp
	src/Window.cpp
	src/Drawer.cpp
	src/GUIDrawer.cpp
    src/SwapChain.cpp
	)

set(RENDER_ENGINE_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/RenderEngine.h
	${RENDER_ENGINE_HEADER_LOCATION}/RenderContext.h
	${RENDER_ENGINE_HEADER_LOCATION}/Window.h
	${RENDER_ENGINE_HEADER_LOCATION}/Drawer.h
	${RENDER_ENGINE_HEADER_LOCATION}/GUIDrawer.h
	${RENDER_ENGINE_HEADER_LOCATION}/SwapChain.h
	)

add_library(RenderEngine STATIC ${RENDER_ENGINE_SRC} ${RENDER_ENGINE_HEADERS})
set_property(TARGET RenderEngine PROPERTY CXX_STANDARD 20)

target_link_libraries(RenderEngine 
	PUBLIC 
		glfw
		Vulkan::Vulkan)
		
target_compile_definitions(RenderEngine 
	PRIVATE 
		VK_USE_PLATFORM_WIN32_KHR
		GLFW_INCLUDE_VULKAN
		GLFW_EXPOSE_NATIVE_WIN32)
target_include_directories(RenderEngine 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
		${GENERATED_INCLUDE_DIR})
