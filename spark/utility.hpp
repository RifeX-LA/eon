#pragma once

#include <functional>
#include <iterator>
#include <memory> // std::addressof
#include <ranges>
#include <utility>

#include <eon/concepts.hpp>
#include <eon/nullable.hpp>
#include <eon/utility.hpp>

namespace eon::spark {

namespace detail::get_ {

void get(); // MSVC ADL workaround

template <typename T, std::size_t I>
concept has_member = requires(T && value) { std::forward<T>(value).template get<I>(); };

template <typename T, std::size_t I>
concept has_adl = requires(T && value) { get<I>(std::forward<T>(value)); };

template <std::size_t I>
class cpo final {
    enum class strategy : std::uint8_t { none, member, adl };

    template <typename T>
    [[nodiscard]] static consteval eon::detail::choice_t<strategy> choose() noexcept {
        if constexpr (has_member<T, I>) {
            return {strategy::member, noexcept(std::declval<T>().template get<I>())};
        } else if constexpr (has_adl<T, I>) {
            return {strategy::adl, noexcept(get<I>(std::declval<T>()))};
        } else {
            return {strategy::none};
        }
    }

    template <typename T>
    static constexpr auto choice = choose<T>();

public:
    template <typename T>
    requires (choice<T>.strategy != strategy::none)
    [[nodiscard]] constexpr decltype(auto) operator()(T && value) const noexcept(choice<T>.nothrow) {
        static constexpr auto strat = choice<T>.strategy;

        if constexpr (strat == strategy::member) {
            return std::forward<T>(value).template get<I>();
        } else if constexpr (strat == strategy::adl) {
            return get<I>(std::forward<T>(value));
        } else {
            static_assert(always_false<T>, "Unexpected strategy");
        }
    }
};

} // namespace detail::get_

template <std::size_t I>
inline constexpr detail::get_::cpo<I> get;

inline constexpr detail::get_::cpo<0> key;
inline constexpr detail::get_::cpo<1> value;

struct indirect_t final {
    template <indirectable T>
    [[nodiscard]] constexpr decltype(auto) operator()(T && value) const noexcept(noexcept(*std::forward<T>(value))) {
        return *std::forward<T>(value);
    }
};

inline constexpr indirect_t indirect;

struct addressof_t final {
    template <typename T>
    [[nodiscard]] constexpr auto operator()(T & value) const noexcept {
        return std::addressof(value);
    }
};

inline constexpr addressof_t addressof;

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
    [[nodiscard]] constexpr auto operator()(T && value) const
        noexcept(noexcept(std::hash<std::remove_cvref_t<T>>{}(std::forward<T>(value)))) {
        return std::hash<std::remove_cvref_t<T>>{}(std::forward<T>(value));
    }
};

inline constexpr hash_t hash;

namespace detail::front_ {

template <typename Rng>
concept has_member = requires(Rng && rng) { std::forward<Rng>(rng).front(); };

class cpo final {
    enum class strategy : bool {
        member,
        begin,
    };

    template <typename Rng>
    [[nodiscard]] static consteval eon::detail::choice_t<strategy> choose() noexcept {
        if constexpr (has_member<Rng>) {
            return {strategy::member, noexcept(std::declval<Rng>().front())};
        } else {
            return {strategy::begin, noexcept(*std::ranges::begin(std::declval<Rng &>()))};
        }
    }

    template <typename Rng>
    static constexpr auto choice = choose<Rng>();

public:
    template <std::ranges::forward_range Rng>
    [[nodiscard]] constexpr decltype(auto) operator()(Rng && rng) const noexcept(choice<Rng>.nothrow) {
        static constexpr auto strat = choice<Rng>.strategy;

        if constexpr (strat == strategy::member) {
            return std::forward<Rng>(rng).front();
        } else if constexpr (strat == strategy::begin) {
            return *std::ranges::begin(rng);
        } else {
            static_assert(always_false<Rng>, "Unexpected strategy");
        }
    }
};

} // namespace detail::front_

inline constexpr detail::front_::cpo front;

namespace detail::back_ {

template <typename Rng>
concept has_member = requires(Rng && rng) { std::forward<Rng>(rng).back(); };

template <typename Rng>
concept can_prev_end = std::ranges::common_range<Rng> && std::ranges::bidirectional_range<Rng>;

class cpo final {
    enum class strategy : std::uint8_t {
        member,
        prev_end,
        next_begin,
        pass_to_prev_end,
    };

    template <typename It, std::sentinel_for<It> Sent>
    [[nodiscard]] static constexpr It
    find_prev_end(It it, Sent const sent) noexcept(std::is_nothrow_copy_constructible_v<It> &&
                                                   std::is_nothrow_copy_assignable_v<It> && noexcept(++it == sent)) {
        for (auto next = std::ranges::next(it); next != sent; ++it, ++next) {
        }
        return it;
    }

    template <typename Rng>
    [[nodiscard]] static consteval eon::detail::choice_t<strategy> choose() noexcept {
        if constexpr (has_member<Rng>) {
            return {strategy::member, noexcept(std::declval<Rng>().back())};
        } else if constexpr (can_prev_end<Rng>) {
            return {strategy::prev_end, noexcept(*std::ranges::prev(std::ranges::end(std::declval<Rng &>())))};
        } else if constexpr (std::ranges::sized_range<Rng>) {
            return {strategy::next_begin, noexcept(*std::ranges::next(std::ranges::begin(std::declval<Rng &>()),
                                                                      std::ranges::size(std::declval<Rng &>()) - 1))};
        } else {
            return {strategy::pass_to_prev_end, noexcept(*find_prev_end(std::ranges::begin(std::declval<Rng &>()),
                                                                        std::ranges::end(std::declval<Rng &>())))};
        }
    }

    template <typename Rng>
    static constexpr auto choice = choose<Rng>();

public:
    template <std::ranges::forward_range Rng>
    [[nodiscard]] constexpr decltype(auto) operator()(Rng && rng) const noexcept(choice<Rng>.nothrow) {
        static constexpr auto strat = choice<Rng>.strategy;

        if constexpr (strat == strategy::member) {
            return std::forward<Rng>(rng).back();
        } else if constexpr (strat == strategy::prev_end) {
            return *std::ranges::prev(std::ranges::end(rng));
        } else if constexpr (strat == strategy::next_begin) {
            return *std::ranges::next(std::ranges::begin(rng), std::ranges::size(rng) - 1);
        } else if constexpr (strat == strategy::pass_to_prev_end) {
            return *find_prev_end(std::ranges::begin(rng), std::ranges::end(rng));
        } else {
            static_assert(always_false<Rng>, "Unexpected strategy");
        }
    }
};

} // namespace detail::back_

inline constexpr detail::back_::cpo back;

struct invoke_t final {
    template <typename... Args, std::invocable<Args...> Fn>
    decltype(auto) operator()(Fn && fn, Args &&... args) const noexcept(std::is_nothrow_invocable_v<Fn, Args...>) {
        return std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};

inline constexpr invoke_t invoke;

struct range_from_nullable_t final {
    template <nullable T>
    requires (std::ranges::range<nullable_value_t<T>>)
    [[nodiscard]] constexpr std::ranges::range auto operator()(T && value) const {
        using rng_t = nullable_value_t<T>;
        using subrange_t = std::ranges::subrange<std::ranges::iterator_t<rng_t>, std::ranges::sentinel_t<rng_t>>;

        if (value) {
            auto && rng = *std::forward<T>(value);
            return subrange_t(std::ranges::begin(rng), std::ranges::end(rng));
        }
        return subrange_t{};
    }
};

inline constexpr range_from_nullable_t range_from_nullable;

template <typename Proj, typename Fn>
class proj final {
public:
    constexpr proj(Proj proj, Fn fn) noexcept(std::is_nothrow_move_constructible_v<Proj> &&
                                              std::is_nothrow_move_constructible_v<Fn>)
        : _proj(std::move(proj)), _fn(std::move(fn)) {
    }

    template <typename... Args>
    requires ((std::invocable<Proj const &, Args &&> && ...) &&
              std::invocable<Fn const &, std::invoke_result_t<Proj const &, Args &&>...>)
    constexpr decltype(auto) operator()(Args &&... args) const
        noexcept((std::is_nothrow_invocable_v<Proj const &, Args &&> && ...) &&
                 std::is_nothrow_invocable_v<Fn const &, std::invoke_result_t<Proj const &, Args &&>...>) {
        return std::invoke(_fn, std::invoke(_proj, std::forward<Args>(args))...);
    }

private:
    Proj _proj;
    Fn _fn;
};

template <typename T>
class always final {
public:
    explicit constexpr always(T const & val) noexcept(std::is_nothrow_copy_constructible_v<T>) : _val(val) {
    }

    explicit constexpr always(T && val) noexcept(std::is_nothrow_move_constructible_v<T>) : _val(std::move(val)) {
    }

    [[nodiscard]] constexpr T const & operator()(auto &&...) const noexcept {
        return _val;
    }

private:
    T _val;
};

struct stub_t final {
    constexpr void operator()(auto &&...) const noexcept {
    }
};

inline constexpr stub_t stub;

} // namespace eon::spark
