#pragma once

#include <render_engine/debug/Topic.h>

namespace RenderEngine::Debug::Topics
{
    struct Synchronization
    {
        static inline PrintDestinationType print_destination{ PrintDestinationType::Console };
        static inline const std::optional<ConsoleColor> console_color = std::nullopt;
        static constexpr const bool enabled{ false };
    };
    static_assert(Topic<Synchronization>, "A debug topic must fulfill the requirements");
}