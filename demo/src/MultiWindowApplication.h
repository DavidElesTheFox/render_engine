#pragma once


#include <imgui.h>
#include <render_engine/RenderContext.h>
#include <render_engine/Device.h>
#include <render_engine/RendererFactory.h>
#include <render_engine/renderers/ExampleRenderer.h>
#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/assets/Mesh.h>

#define ENABLE_WINDOW_0 true
#define ENABLE_WINDOW_1 true
#define ENABLE_WINDOW_2 true

class MultiWindowApplication
{
public:
	void init();

	void run();

private:

	void initEngine();
	std::unique_ptr<RenderEngine::Window> _window_0;
	std::unique_ptr<RenderEngine::Window> _window_1;
	std::unique_ptr<RenderEngine::Window> _window_2;
};
