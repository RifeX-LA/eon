#pragma once

#include <functional>
#include <algorithm>
#include <concepts>
#include <type_traits>
#include <optional>
#include <vector>
#include <ranges>

#include <eon/concepts.hpp>
#include <eon/spark/utility.hpp>

namespace eon {

    template <typename Fn>
    requires (std::invocable<Fn> && eon::decayed<Fn>)
    class scope_guard final {
        Fn m_fn;

    public:
        constexpr scope_guard()
        noexcept(std::is_nothrow_default_constructible_v<Fn>)
        requires(std::default_initializable<Fn>)
        : m_fn() {}

        constexpr explicit scope_guard(Fn fn)
        noexcept(std::is_nothrow_move_constructible_v<Fn>)
        requires(std::move_constructible<Fn>)
        : m_fn(std::move(fn)) {}

        constexpr ~scope_guard() noexcept {
            std::invoke(std::move(m_fn));
        }

    public:
        scope_guard(scope_guard const &) = delete;
        scope_guard(scope_guard &&) = delete;
        scope_guard & operator=(scope_guard const &) = delete;
        scope_guard & operator=(scope_guard &&) = delete;
    };


    template <typename Fn>
    requires (std::invocable<Fn> && eon::decayed<Fn>)
    class cancelable_scope_guard final {
        std::optional<Fn> m_fn;

    public:
        constexpr cancelable_scope_guard()
        noexcept(std::is_nothrow_default_constructible_v<Fn>)
        requires(std::default_initializable<Fn>)
        : m_fn(std::in_place) {}

        constexpr explicit cancelable_scope_guard(Fn fn)
        noexcept(std::is_nothrow_move_constructible_v<Fn>)
        requires(std::move_constructible<Fn>)
        : m_fn(std::move(fn)) {}

        constexpr ~cancelable_scope_guard() noexcept {
            if (m_fn) {
                std::invoke(std::move(*m_fn));
            }
        }

    public:
        cancelable_scope_guard(cancelable_scope_guard const &) = delete;
        cancelable_scope_guard(cancelable_scope_guard &&) = delete;
        cancelable_scope_guard & operator=(cancelable_scope_guard const &) = delete;
        cancelable_scope_guard & operator=(cancelable_scope_guard &&) = delete;

    public:
        constexpr void reset() noexcept {
            m_fn.reset();
        }

        constexpr void release() noexcept(std::is_nothrow_invocable_v<Fn>) {
            std::invoke(std::move(*m_fn));
            reset();
        }
    };


    class scope_guard_container final {
        std::vector<std::move_only_function<void()>> m_guards;

    public:
        constexpr scope_guard_container() = default;

        template <std::ranges::input_range Rng>
        requires (std::invocable<std::ranges::range_value_t<Rng>>)
        constexpr explicit scope_guard_container(Rng && rng) {
            m_guards.assign_range(std::forward<Rng>(rng));
        }

        constexpr ~scope_guard_container() noexcept {
            std::ranges::for_each(m_guards, spark::invoke);
        }

    public:
        scope_guard_container(scope_guard_container const &) = delete;
        scope_guard_container(scope_guard_container &&) = default;
        scope_guard_container & operator=(scope_guard_container const &) = delete;
        scope_guard_container & operator=(scope_guard_container &&) = default;

    public:
        template <std::invocable Fn>
        constexpr void push_back(Fn && fn) {
            m_guards.emplace_back(std::forward<Fn>(fn));
        }

        template <std::ranges::input_range Rng>
        requires (std::invocable<std::ranges::range_value_t<Rng>>)
        constexpr void append_range(Rng && rng) {
            m_guards.append_range(std::forward<Rng>(rng));
        }

        constexpr void reserve(std::size_t const size) {
            m_guards.reserve(size);
        }

        constexpr void reset() noexcept {
            m_guards.clear();
        }

        constexpr void release() {
            std::ranges::for_each(m_guards, spark::invoke);
            reset();
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return m_guards.empty();
        }
    };

}