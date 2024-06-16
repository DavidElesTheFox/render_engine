#pragma once
#include <functional>

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
        T::enabled;
        { Internal::is_constexpr_enabled<T>() };
    };

}