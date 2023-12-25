#pragma once

#include <memory>
#include <ranges>
namespace RenderEngine::views
{
    struct to_raw_pointer_fn
    {
        template<typename Rng>
        constexpr auto operator()(Rng&& rng) const noexcept
        {
            return std::ranges::transform_view(std::forward<Rng>(rng), [](auto& ptr) { return ptr.get(); });
        }

        template <typename Rng>
        friend auto operator|(Rng&& rng, const to_raw_pointer_fn& view)
        {
            return view(std::ranges::views::all(std::forward<Rng>(rng)));
        }
    };

    constexpr to_raw_pointer_fn to_raw_pointer;
}