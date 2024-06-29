#include <render_engine/Debugger.h>

#include <algorithm>
#include <iostream>

namespace RenderEngine
{
    Debugger::GuiCallbackToken::~GuiCallbackToken()
    {
        _debugger.removeCallback(_callback);
    }

    Debugger::GuiCallbackToken Debugger::addGuiCallback(std::function<void()> callback)
    {
        _on_gui_callbacks.push_back(std::move(callback));
        return Debugger::GuiCallbackToken(*this, _on_gui_callbacks.back());
    }
    void Debugger::callGuiCallbacks() const
    {
        for (const auto& callback : _on_gui_callbacks)
        {
            callback();
        }
    }
    void Debugger::removeCallback(std::function<void()>& callback)
    {
        std::erase_if(_on_gui_callbacks, [&](auto& stored_callback) { return &stored_callback == &callback; });
    }
    void Debugger::printToConsole(const std::optional<Debug::ConsoleColor>& color, const std::string_view& msg)
    {
        _console.println(color, std::format("{:s} {:s}", getLogPrefix(), msg));
    }

    std::string Debugger::getLogPrefix() const
    {
        using namespace std::chrono;
        const auto time_spent = Clock::now() - _start_time;
        return std::format("[{:%T}]", time_spent);
    }


}