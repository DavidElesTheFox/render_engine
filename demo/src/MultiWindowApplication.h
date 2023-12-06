#pragma once


#include <imgui.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/containers/ImageStream.h>
#include <render_engine/Device.h>
#include <render_engine/RenderContext.h>
#include <render_engine/RendererFactory.h>
#include <render_engine/renderers/ExampleRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

#define ENABLE_WINDOW_0 true
#define ENABLE_WINDOW_1 true
#define ENABLE_WINDOW_2 true
#define ENABLE_WINDOW_3 true

class MultiWindowApplication
{
public:
    void init();

    void run();

private:

    void initEngine();
    void initImages();
    void updateImageStream();
    std::unique_ptr<RenderEngine::Window> _window_0;
    std::unique_ptr<RenderEngine::Window> _window_1;
    std::unique_ptr<RenderEngine::Window> _window_2;
    std::unique_ptr<RenderEngine::Window> _window_3;
    std::unique_ptr<RenderEngine::ImageStream> _image_stream;
    std::vector<RenderEngine::Image> _images;
    size_t _current_image{ 0 };
    std::chrono::steady_clock::time_point _last_image_update{ };
};
