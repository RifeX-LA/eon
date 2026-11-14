#pragma once

#include "eon/tpl/functions.hpp"
#include <iomanip>
#include <syncstream>

#include <eon/tpl/detail/io_ops.hpp>
#include <eon/tpl/detail/quoted_char.hpp>
#include <type_traits>

namespace eon::tpl::detail {

template <tuple_like Tuple, string_like Sep>
struct io_view;

template <typename T>
[[nodiscard]] constexpr decltype(auto) quoted(T && value) noexcept {
    if constexpr (string_like<T>) {
        return std::quoted(value);
    } else if constexpr (character<T>) {
        return quoted_char<T>{std::forward<T>(value), std::remove_cvref_t<T>('\'')};
    } else if constexpr (tuple_like<T>) {
        return io_view{value, ", "};
    } else {
        return std::forward<T>(value);
    }
}

template <character CharT, typename Traits, tuple_like Tuple, string_like Sep>
void print(std::basic_ostream<CharT, Traits> & os, Tuple && tuple, Sep const & sep) {
    static constexpr std::size_t tuple_size = std::tuple_size_v<std::remove_cvref_t<Tuple>>;

    for_each_tuple_index<Tuple>([&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
        os << detail::quoted(std::get<I>(std::forward<Tuple>(tuple)));
        if constexpr (I + 1 < tuple_size) {
            os << sep;
        }
    });
}

template <character CharT, typename Traits, tuple_like Tuple, string_like Sep>
bool read(std::basic_istream<CharT, Traits> & is, Tuple && tuple, Sep const & sep) {
    static constexpr std::size_t tuple_size = std::tuple_size_v<std::remove_cvref_t<Tuple>>;

    bool ok = true;

    for_each_tuple_index<Tuple>([&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
        if (!ok) {
            return;
        }

        is >> detail::quoted(std::get<I>(std::forward<Tuple>(tuple)));
        if (is.fail()) {
            ok = false;
            return;
        }

        if constexpr (I + 1 < tuple_size) {
            ok &= accept_range(is, sep);
        }
    });

    return ok;
}

template <tuple_like Tuple, string_like Sep>
struct io_view {
    Tuple & tuple;
    Sep sep;

    explicit constexpr io_view(Tuple & tuple, Sep sep) noexcept(std::is_nothrow_move_constructible_v<Sep>)
        : tuple(tuple), sep(std::move(sep)) {
    }

    io_view(io_view const &) = delete;
    io_view(io_view &&) = delete;
};

template <typename CharT, typename Traits, typename Tuple, typename Sep>
std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & os,
                                               io_view<Tuple, Sep> const & view) {
    std::basic_osyncstream buffer(os);
    buffer << CharT('(');
    detail::print(buffer, view.tuple, view.sep);
    buffer << CharT(')');

    return os;
}

template <typename CharT, typename Traits, typename Tuple, typename Sep>
std::basic_istream<CharT, Traits> & operator>>(std::basic_istream<CharT, Traits> & is,
                                               io_view<Tuple, Sep> const & view) {
    if (detail::accept_char(is, CharT('(')) && detail::read(is, view.tuple, view.sep)) {
        detail::accept_char(is, CharT(')'));
    }

    return is;
}

} // namespace eon::tpl::detail
