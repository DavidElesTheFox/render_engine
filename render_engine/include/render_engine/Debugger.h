#pragma once

#include <render_engine/debug/SyncLogbook.h>
#include <render_engine/debug/Topic.h>

#include <format>
#include <functional>
#include <iostream>
#include <string_view>
#include <vector>
namespace RenderEngine
{

    class Debugger
    {
    private:
        static constexpr const uint32_t sync_logbook_max_stacksize = 128;
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
        template<typename... T>
        void print(const Debug::Topic auto& topic, [[maybe_unused]] std::string_view format, [[maybe_unused]] T&&... args)
        {
            if constexpr (topic.enabled)
            {
                print(topic, std::vformat(format, std::make_format_args(args...)));
            }
        }
        void print(const Debug::Topic auto& topic, [[maybe_unused]] std::string_view msg)
        {
            if constexpr (topic.enabled)
            {
                switch (topic.print_destination)
                {
                    case Debug::PrintDestinationType::Null:
                        break;
                    case Debug::PrintDestinationType::Console:
                        printToConsole(msg);
                        break;
                }
            }
        }
        void callGuiCallbacks() const;

        SyncLogbook& getSyncLogbook() { return _sync_logbook; }
    private:
        void printToConsole(const std::string_view& msg);
        void removeCallback(std::function<void()>& callback);
        std::vector<std::function<void()>> _on_gui_callbacks;
        SyncLogbook _sync_logbook{ sync_logbook_max_stacksize };
    };
}