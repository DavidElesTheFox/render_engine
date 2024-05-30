#pragma once

#include <memory>
#include <ranges>
namespace RenderEngine::views
{
    namespace detail
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

        template<typename Container>
        struct to_fn
        {
            template<typename Rng>
            constexpr auto operator()(Rng&& rng) const noexcept
            {
                return Container(rng.begin(), rng.end());
            }

            template<typename Rng>
            friend auto operator|(Rng&& rng, const to_fn& view)
            {
                return view(std::ranges::views::all(std::forward<Rng>(rng)));
            }
        };

    }

    constexpr detail::to_raw_pointer_fn to_raw_pointer;
    template<typename Container, typename... T>
    constexpr const auto& to(T&&... args)
    {
        constexpr auto num_of_args = sizeof...(T);
        static_assert(num_of_args < 2);

        static detail::to_fn<Container> _to;

        if constexpr (num_of_args == 0)
        {
            return _to;
        }
        else
        {
            return _to(std::forward<T>(args)...);
        }
    }


}

