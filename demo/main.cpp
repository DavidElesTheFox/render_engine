#define SDL_MAIN_HANDLED
#include<iostream>

#include "MultiWindowApplication.h"
#include "DemoApplication.h"

void runMultiWindowApplication()
{
	MultiWindowApplication application;
	application.init();
	application.run();
}

void runDemoApplication()
{
	DemoApplication application;
	application.init();
	application.run();
}

int main()
{

	try
	{
		runDemoApplication();
		RenderEngine::RenderContext::context().reset();
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error occurred: " << exception.what() << std::endl;
		return -1;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << std::endl;
		return -2;
	}
}