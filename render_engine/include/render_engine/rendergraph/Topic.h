#pragma once

#include <render_engine/debug/Topic.h>

namespace RenderEngine::Debug::Topics
{
    struct RenderGraphBuilder
    {
        static inline PrintDestinationType print_destination{ PrintDestinationType::Null };
        static constexpr const bool enabled{ false };
    };

    static_assert(Topic<RenderGraphBuilder>, "A debug topic must fulfill the requirements");

    struct RenderGraphExecution
    {
        static inline PrintDestinationType print_destination{ PrintDestinationType::Null };
        static constexpr const bool enabled{ false };
    };
    static_assert(Topic<RenderGraphExecution>, "A debug topic must fulfill the requirements");

}