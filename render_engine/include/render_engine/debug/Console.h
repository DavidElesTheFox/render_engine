#pragma once

#include <render_engine/debug/ConsoleColor.h>

#include <iostream>
#include <mutex>
#include <optional>

namespace RenderEngine::Debug
{
    class Console
    {
    public:
        Console() = default;

        Console(Console&&) = delete;
        Console(const Console&) = delete;

        Console& operator=(Console&&) = delete;
        Console& operator=(const Console&) = delete;

        void println(std::optional<ConsoleColor> color, const std::string_view& message)
        {
            std::lock_guard lock{ _access_mutex };
            if (color != std::nullopt)
            {
                std::cout << color->apply();
            }
            std::cout << message;
            if (color != std::nullopt)
            {
                std::cout << color->reset();
            }
            std::cout << std::endl;
        }

    private:
        std::mutex _access_mutex;

    };
}