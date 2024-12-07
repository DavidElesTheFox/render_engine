#pragma once

#include <render_engine/debug/Topic.h>

namespace RenderEngine::Debug::Topics
{
    struct ResourceStateValidation
    {
        static inline PrintDestinationType print_destination{ PrintDestinationType::Console };
        static inline const std::optional<ConsoleColor> console_color = ConsoleColors::Red;
        static constexpr const bool enabled{ false };
    };
    static_assert(Topic<ResourceStateValidation>, "A debug topic must fulfill the requirements");
}