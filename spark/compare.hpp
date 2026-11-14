#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include <eon/concepts.hpp>

namespace eon::spark {

namespace detail {

template <std::default_initializable IntegralComp, std::default_initializable FloatingPointComp,
          std::default_initializable CommonComp>
struct make_comparator final {
    template <std::integral T, std::integral U>
    requires (std::predicate<IntegralComp, T, U>)
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept(noexcept(IntegralComp{}(lhs, rhs))) {
        return IntegralComp{}(lhs, rhs);
    }

    template <std::floating_point T, std::floating_point U>
    requires (std::predicate<FloatingPointComp, T, U>)
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept(noexcept(FloatingPointComp{}(lhs, rhs))) {
        return FloatingPointComp{}(lhs, rhs);
    }

    template <typename T, typename U>
    requires ((!std::is_arithmetic_v<std::remove_cvref_t<T>> || !std::is_arithmetic_v<std::remove_cvref_t<U>>) &&
              std::predicate<CommonComp, T, U>)
    [[nodiscard]] constexpr bool operator()(T && lhs, U && rhs) const
        noexcept(noexcept(CommonComp{}(std::forward<T>(lhs), std::forward<U>(rhs)))) {
        return CommonComp{}(std::forward<T>(lhs), std::forward<U>(rhs));
    }
};

struct integral_equal final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return std::cmp_equal(+lhs, +rhs);
    }
};

struct integral_not_equal final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return std::cmp_not_equal(+lhs, +rhs);
    }
};

struct integral_less final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return std::cmp_less(+lhs, +rhs);
    }
};

struct integral_greater final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return std::cmp_greater(+lhs, +rhs);
    }
};

struct integral_less_equal final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return std::cmp_less_equal(+lhs, +rhs);
    }
};

struct integral_greater_equal final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return std::cmp_greater_equal(+lhs, +rhs);
    }
};

struct floating_point_equal final {
    template <std::floating_point T, std::floating_point U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        using common_t = std::common_type_t<T, U>;
        common_t const a = lhs, b = rhs;
        if (a == b) {
            return true; // Includes equal infinities and signed zeros.
        }
        if (!std::isfinite(a) || !std::isfinite(b)) {
            return false;
        }
        // Absolute tolerance near zero, relative tolerance for larger values.
        common_t const scale = std::max({common_t{1}, std::abs(a), std::abs(b)});
        // Opposite signs can overflow a-b even though both operands are finite.
        auto const distance = (a < 0) != (b < 0) ? std::abs(a) / scale + std::abs(b) / scale : std::abs(a - b) / scale;
        return distance <= std::numeric_limits<common_t>::epsilon();
    }
};

struct floating_point_not_equal final {
    template <std::floating_point T, std::floating_point U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return !floating_point_equal{}(lhs, rhs);
    }
};

struct floating_point_less final {
    template <std::floating_point T, std::floating_point U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return lhs < rhs && !floating_point_equal{}(lhs, rhs);
    }
};

struct floating_point_less_equal final {
    template <std::floating_point T, std::floating_point U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return lhs < rhs || floating_point_equal{}(lhs, rhs);
    }
};

struct floating_point_greater final {
    template <std::floating_point T, std::floating_point U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return floating_point_less{}(rhs, lhs);
    }
};

struct floating_point_greater_equal final {
    template <std::floating_point T, std::floating_point U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return floating_point_less_equal{}(rhs, lhs);
    }
};

} // namespace detail

using equal = detail::make_comparator<detail::integral_equal, detail::floating_point_equal, std::equal_to<>>;
using not_equal =
    detail::make_comparator<detail::integral_not_equal, detail::floating_point_not_equal, std::not_equal_to<>>;
using less = detail::make_comparator<detail::integral_less, detail::floating_point_less, std::less<>>;
using less_equal =
    detail::make_comparator<detail::integral_less_equal, detail::floating_point_less_equal, std::less_equal<>>;
using greater = detail::make_comparator<detail::integral_greater, detail::floating_point_greater, std::greater<>>;
using greater_equal =
    detail::make_comparator<detail::integral_greater_equal, detail::floating_point_greater_equal, std::greater_equal<>>;

struct divisible final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return lhs % rhs == 0;
    }
};

struct not_divisible final {
    template <std::integral T, std::integral U>
    [[nodiscard]] constexpr bool operator()(T lhs, U rhs) const noexcept {
        return lhs % rhs != 0;
    }
};

template <typename T, std::default_initializable Comp>
class bind2nd_comparator final {
public:
    explicit constexpr bind2nd_comparator(T const & rhs) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires (std::copy_constructible<T>)
        : _rhs(rhs) {
    }

    explicit constexpr bind2nd_comparator(T && rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
    requires (std::move_constructible<T>)
        : _rhs(std::move(rhs)) {
    }

    template <typename U>
    [[nodiscard]] constexpr bool operator()(U const & lhs) const noexcept(noexcept(Comp{}(lhs, _rhs)))
    requires (std::predicate<Comp, U, T>)
    {
        return Comp{}(lhs, _rhs);
    }

private:
    T _rhs;
};

template <typename T>
using eq = bind2nd_comparator<T, equal>;

template <typename T>
using neq = bind2nd_comparator<T, not_equal>;

template <typename T>
using lt = bind2nd_comparator<T, less>;

template <typename T>
using le = bind2nd_comparator<T, less_equal>;

template <typename T>
using gt = bind2nd_comparator<T, greater>;

template <typename T>
using ge = bind2nd_comparator<T, greater_equal>;

template <std::integral T>
using div = bind2nd_comparator<T, divisible>;

template <std::integral T>
using ndiv = bind2nd_comparator<T, not_divisible>;

inline constexpr eq zero(0);
inline constexpr neq non_zero(0);
inline constexpr lt negative(0);
inline constexpr gt positive(0);
inline constexpr ge non_negative(0);
inline constexpr le non_positive(0);
inline constexpr div even(2);
inline constexpr ndiv odd(2);

} // namespace eon::spark
