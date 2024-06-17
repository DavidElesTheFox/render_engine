#pragma once

namespace RenderEngine::Debug
{
    class ConsoleColor
    {
    public:
        constexpr ConsoleColor(const char* color_code)
            : _code(color_code)
        {}
        constexpr const char* apply() const { return _code; }
        constexpr const char* reset() const { return _color_reset; }
    private:
        const char* _code;
        static constexpr auto _color_reset = "\033[0m";
    };
    namespace ConsoleColors
    {
        constexpr const ConsoleColor Default{ "\033[0m" };
        constexpr const ConsoleColor Black{ "\033[30m" };
        constexpr const ConsoleColor Red{ "\033[31m" };
        constexpr const ConsoleColor Green{ "\033[32m" };
        constexpr const ConsoleColor Yellow{ "\033[33m" };
        constexpr const ConsoleColor Blue{ "\033[34m" };
        constexpr const ConsoleColor Magenta{ "\033[35m" };
        constexpr const ConsoleColor Cyan{ "\033[36m" };
        constexpr const ConsoleColor White{ "\033[37m" };
    }
}