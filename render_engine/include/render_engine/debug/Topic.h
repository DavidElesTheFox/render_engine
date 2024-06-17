#pragma once
#include <functional>
#include <optional>

#include <render_engine/debug/ConsoleColor.h>
namespace RenderEngine::Debug
{
    enum class PrintDestinationType
    {
        Null,
        Console
    };

    namespace Internal
    {
        template<typename T>
        consteval void is_constexpr_enabled()
        {
            bool x = T::enabled;
        }
    }

    template<typename T>
    concept Topic = requires(const T & topic)
    {
        { topic.print_destination } -> std::convertible_to<PrintDestinationType>;
        { topic.console_color } -> std::convertible_to<std::optional<ConsoleColor>>;
        T::enabled;
        { Internal::is_constexpr_enabled<T>() };
    };

}