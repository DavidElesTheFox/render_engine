#include<iostream>
#include <render_engine/RenderContext.h>
#include <render_engine/RenderEngine.h>

int main()
{
	{
		auto window = RenderEngine::RenderContext::context().getEngine(0).createWindow("Main Window");
	}
	RenderEngine::RenderContext::context().reset();
	return 0;
}