#pragma once

#include <cmath>
#include <functional>
#include <type_traits>

namespace eon::spark {

template <typename T>
[[nodiscard]] constexpr auto add(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::plus{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto sub(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::minus{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto mult(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::multiplies{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto divides(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::divides{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto remainder(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    static constexpr auto remainder = []<typename U, typename Y>(U && lhs, Y && rhs) {
        return std::forward<U>(lhs) % std::forward<Y>(rhs);
    };
    return std::bind_back(remainder, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto bit_and(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::bit_and{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto bit_or(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::bit_or{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto bit_xor(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::bind_back(std::bit_xor{}, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto lshift(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    static constexpr auto bit_shift_left = []<typename U, typename Y>(U && lhs, Y && rhs) {
        return std::forward<U>(lhs) << std::forward<Y>(rhs);
    };
    return std::bind_back(bit_shift_left, std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr auto rshift(T && value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    static constexpr auto bit_shift_right = []<typename U, typename Y>(U && lhs, Y && rhs) {
        return std::forward<U>(lhs) >> std::forward<Y>(rhs);
    };
    return std::bind_back(bit_shift_right, std::forward<T>(value));
}

template <typename Op>
[[nodiscard]] constexpr auto assign(Op && op) noexcept(std::is_nothrow_constructible_v<std::decay_t<Op>, Op>) {
    return
        [op = std::forward<Op>(op)]<typename T>(T && lhs) -> decltype(auto) { return lhs = op(std::forward<T>(lhs)); };
}

} // namespace eon::spark
