#pragma once

#include <ranges>
#include <utility>
#include <iterator>
#include <functional>
#include <memory>  // std::addressof

#include <eon/concepts.hpp>

namespace eon::spark {

    struct indirect_t final {
        template <indirectable T>
        [[nodiscard]] constexpr decltype(auto) operator()(T && value) const
        noexcept(noexcept(*std::forward<T>(value))) {
            return *std::forward<T>(value);
        }
    };

    inline constexpr indirect_t indirect;


    struct addressof_t final {
        template <typename T>
        [[nodiscard]] constexpr auto operator()(T && value) const noexcept {
            return std::addressof(std::forward<T>(value));
        }
    };

    inline constexpr addressof_t addressof;


    template <std::size_t I>
    struct get_t final {
        template <typename T>
        [[nodiscard]] constexpr decltype(auto) operator()(T && value) const
        noexcept(noexcept(std::get<I>(std::forward<T>(value)))) {
            return std::get<I>(std::forward<T>(value));
        }
    };

    template <std::size_t I>
    inline constexpr get_t<I> get;


    struct to_underlying_t final {
        template <typename T>
        requires (std::is_enum_v<T>)
        [[nodiscard]] constexpr auto operator()(T value) const noexcept {
            return std::to_underlying(value);
        }
    };

    inline constexpr to_underlying_t to_underlying;


    struct hash_t final {
        template <typename T>
        [[nodiscard]] constexpr auto operator()(T && value) const noexcept {
            return std::hash<std::remove_cvref_t<T>>{}(value);
        }
    };

    inline constexpr hash_t hash;


    struct front_t final {
        template <std::ranges::input_range Rng>
        [[nodiscard]] constexpr decltype(auto) operator()(Rng && rng) const {
            if constexpr (requires { std::forward<Rng>(rng).front(); }) {
                return std::forward<Rng>(rng).front();
            }
            else {
                return *std::ranges::begin(rng);
            }
        }
    };

    inline constexpr front_t front;


    struct back_t final {
        template <std::ranges::forward_range Rng>
        [[nodiscard]] constexpr decltype(auto) operator()(Rng && rng) const {
            if constexpr (requires { std::forward<Rng>(rng).back(); }) {
                return std::forward<Rng>(rng).back();
            }
            else if constexpr (std::ranges::common_range<Rng> && std::ranges::bidirectional_range<Rng>) {
                return *std::ranges::prev(std::ranges::end(rng));
            }
            else {
                return *std::ranges::next(std::ranges::begin(rng), std::ranges::distance(rng) - 1);
            }
        }
    };

    inline constexpr back_t back;


    struct invoke_t final {
        template <typename... Args, std::invocable<Args...> Fn>
        decltype(auto) operator()(Fn && fn, Args &&... args) const
        noexcept(std::is_nothrow_invocable_v<Fn, Args...>) {
            return std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    };

    inline constexpr invoke_t invoke;
}
