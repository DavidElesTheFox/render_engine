#pragma once
#include <render_engine/window/WindowTunnel.h>

class IWindowSetup
{
public:
    virtual ~IWindowSetup() = default;
    virtual RenderEngine::IWindow& getRenderingWindow() = 0;
    virtual RenderEngine::Window& getUiWindow() = 0;
    virtual void update() = 0;
    virtual uint32_t getBackbufferCount() const = 0;
};

class SingleWindowSetup : public IWindowSetup
{
public:
    SingleWindowSetup(std::vector<uint32_t> renderer_ids);
    ~SingleWindowSetup() override = default;
    RenderEngine::IWindow& getRenderingWindow() override { return *_window; }
    RenderEngine::Window& getUiWindow() override { return *_window; };
    void update() override { _window->update(); }
    uint32_t getBackbufferCount() const override { return 3; }
    uint32_t getParallelFrameCount() const { return 1; }
private:
    std::unique_ptr<RenderEngine::Window> _window;
};

class OffScreenWindowSetup : public IWindowSetup
{
public:
    OffScreenWindowSetup(std::vector<uint32_t> renderer_ids);
    ~OffScreenWindowSetup() override = default;
    RenderEngine::IWindow& getRenderingWindow() override { return _window_tunnel->getOriginWindow(); }
    RenderEngine::Window& getUiWindow() override
    {
        return static_cast<RenderEngine::Window&>(_window_tunnel->getDestinationWindow());
    }
    void update() override { _window_tunnel->update(); }
    uint32_t getBackbufferCount() const override { return 3; }
private:
    std::unique_ptr<RenderEngine::WindowTunnel> _window_tunnel;
};