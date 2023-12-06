#include "render_engine/window/WindowTunnel.h"

namespace RenderEngine
{
    void WindowTunnel::update()
    {
        _origin_window->update();
        _destination_window->update();
    }
}