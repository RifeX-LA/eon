#pragma once

#include <iomanip>

#include <eon/concepts.hpp>
#include <eon/tpl/detail/quoted_char.hpp>

namespace eon::tpl::detail {

template <character CharT, typename Traits, typename Alloc>
[[nodiscard]] auto quoted(std::basic_string<CharT, Traits, Alloc> & s) noexcept {
    return std::quoted(s);
}

template <character CharT, typename Traits, typename Alloc>
[[nodiscard]] auto quoted(std::basic_string<CharT, Traits, Alloc> const & s) noexcept {
    return std::quoted(s);
}

template <character CharT>
[[nodiscard]] auto quoted(CharT const * s) noexcept {
    return std::quoted(s);
}

template <character CharT, typename Traits>
[[nodiscard]] auto quoted(std::basic_string_view<CharT, Traits> sv) noexcept {
    return std::quoted(sv);
}

template <character CharT>
[[nodiscard]] auto quoted(CharT && c) noexcept {
    return quoted_char<CharT>{std::forward<CharT>(c), std::remove_cvref_t<CharT>('\'')};
}

template <typename T>
requires (!character<T>)
[[nodiscard]] decltype(auto) quoted(T && value) noexcept {
    return std::forward<T>(value);
}

} // namespace eon::tpl::detail
