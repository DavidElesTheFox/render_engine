#pragma once

#include <render_engine/renderers/ImageStreamRenderer.h>
#include <render_engine/window/OffScreenWindow.h>

namespace RenderEngine
{
    class WindowTunnel
    {
    public:
        WindowTunnel(std::unique_ptr<OffScreenWindow> origin_window,
                     std::unique_ptr<IWindow> destination_window)
            : _origin_window(std::move(origin_window))
            , _destination_window(std::move(destination_window))
        {
            _origin_window->registerTunnel(*this);
            _destination_window->registerTunnel(*this);
        }
        ~WindowTunnel() = default;

        WindowTunnel(const WindowTunnel&) = delete;
        WindowTunnel(WindowTunnel&&) = default;

        WindowTunnel& operator=(const WindowTunnel&) = delete;
        WindowTunnel& operator=(WindowTunnel&&) = default;

        OffScreenWindow& getOriginWindow() { return *_origin_window; }
        IWindow& getDestinationWindow() { return *_destination_window; }

        const OffScreenWindow& getOriginWindow() const { return *_origin_window; }
        const IWindow& getDestinationWindow() const { return *_destination_window; }

        void update();
    private:
        std::unique_ptr<OffScreenWindow> _origin_window;
        std::unique_ptr<IWindow> _destination_window;
    };
}