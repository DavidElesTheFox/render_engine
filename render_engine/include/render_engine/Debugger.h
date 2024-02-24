#pragma once

#include <format>
#include <functional>
#include <string_view>
#include <vector>

namespace RenderEngine
{
    class Debugger
    {
    public:
        class GuiCallbackToken
        {
        public:
            GuiCallbackToken(Debugger& debugger, std::function<void()> callback)
                : _debugger(debugger)
                , _callback(callback)
            {}
            GuiCallbackToken(const GuiCallbackToken&) = delete;
            GuiCallbackToken(GuiCallbackToken&&) = default;

            GuiCallbackToken& operator=(const GuiCallbackToken&) = delete;
            GuiCallbackToken& operator=(GuiCallbackToken&&) = default;
            ~GuiCallbackToken();
        private:
            Debugger& _debugger;
            std::function<void()>& _callback;
        };

        Debugger() = default;
        ~Debugger() = default;

        Debugger(Debugger&&) = delete;
        Debugger(const Debugger&) = delete;
        Debugger& operator=(Debugger&&) = delete;
        Debugger& operator=(const Debugger&) = delete;

        GuiCallbackToken addGuiCallback(std::function<void()> callback);
        void print(std::string_view msg);
        template<typename... T>
        void print(std::string_view format, T&&... args)
        {
            print(std::vformat(format, std::make_format_args(args...)));
        }

        void callGuiCallbacks() const;
    private:
        void removeCallback(std::function<void()>& callback);
        std::vector<std::function<void()>> _on_gui_callbacks;
    };
}