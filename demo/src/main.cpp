#define SDL_MAIN_HANDLED
#include<iostream>

#include "DemoApplication.h"
#include "MultiWindowApplication.h"
#include "OffScreenTestApplication.h"
#include "ParallelDemoApplication.h"
#include "VolumeRendererDemo.h"

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

void runParallelDemoApplication()
{
    ParallelDemoApplication::Description description
    {
        .backbuffer_count = 3,
        .thread_count = 3,
        .window_width = 1024,// TODO do not change it until the API is not ready
        .window_height = 764,
        .window_title = "DemoWindow",
        .rendering_type = ParallelDemoApplication::RenderingType::Offscreen
    };
    ParallelDemoApplication application(description);
    application.init();
    application.run();
}

void runOffScreenApplication()
{
    OffScreenTestApplication application;
    application.init();
    application.run();
}
void runVolumeRendererDemo()
{
    VolumeRendererDemo application;
    constexpr bool use_ao = true;
    application.init(use_ao);
    application.run();
}

int main()
{
    try
    {
        runParallelDemoApplication();
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