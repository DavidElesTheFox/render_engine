cmake_minimum_required(VERSION 3.18)

find_package(Vulkan REQUIRED)

set(RENDER_ENGINE_HEADER_LOCATION include/render_engine)

set(RENDER_ENGINE_SRC 
	src/RenderEngine.cpp
	src/RenderContext.cpp
	src/Window.cpp
	src/Drawer.cpp
	src/GUIDrawer.cpp
	)

set(RENDER_ENGINE_HEADERS 
	${RENDER_ENGINE_HEADER_LOCATION}/RenderEngine.h
	${RENDER_ENGINE_HEADER_LOCATION}/RenderContext.h
	${RENDER_ENGINE_HEADER_LOCATION}/Window.h
	${RENDER_ENGINE_HEADER_LOCATION}/Drawer.h
	${RENDER_ENGINE_HEADER_LOCATION}/GUIDrawer.h
	)

add_library(RenderEngine STATIC ${RENDER_ENGINE_SRC} ${RENDER_ENGINE_HEADERS})
set_property(TARGET RenderEngine PROPERTY CXX_STANDARD 20)

target_link_libraries(RenderEngine 
	PUBLIC 
		SDL2::SDL2
		Vulkan::Vulkan)
		

target_include_directories(RenderEngine PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)