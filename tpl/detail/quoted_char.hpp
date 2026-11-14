#pragma once

#include <istream>
#include <ostream>
#include <type_traits>
#include <utility>

#include <eon/concepts.hpp>
#include <eon/tpl/detail/io_ops.hpp>

namespace eon::tpl::detail {

template <character CharT>
struct quoted_char final {
    CharT symbol;
    std::remove_cvref_t<CharT> delim;

    constexpr quoted_char(CharT c, std::remove_cvref_t<CharT> delim) noexcept
        : symbol(std::forward<CharT>(c)), delim(delim) {
    }

    quoted_char(quoted_char &&) = delete;
    quoted_char(quoted_char const &) = delete;
};

template <character CharT>
quoted_char(CharT &&, std::remove_cvref_t<CharT>) -> quoted_char<CharT>;

template <typename CharT, typename Traits, typename Symbol>
std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & os,
                                               quoted_char<Symbol> const & view) {
    os.put(view.delim);
    if (view.symbol == view.delim || view.symbol == CharT('\\')) {
        os.put(CharT('\\'));
    }
    return os.put(view.symbol).put(view.delim);
}

template <typename CharT, typename Traits, typename Symbol>
requires (std::is_lvalue_reference_v<Symbol> && !std::is_const_v<std::remove_reference_t<Symbol>>)
std::basic_istream<CharT, Traits> & operator>>(std::basic_istream<CharT, Traits> & is,
                                               quoted_char<Symbol> const & view) {
    if (!accept_char(is, view.delim)) {
        return is;
    }
    CharT symbol{};
    if (!is.get(symbol)) {
        return is;
    }
    if (symbol == CharT('\\')) {
        if (!is.get(symbol)) {
            return is;
        }
    } else if (symbol == view.delim) {
        is.setstate(std::ios_base::failbit); // Empty quoted value.
        return is;
    }
    if (accept_char(is, view.delim)) {
        view.symbol = symbol;
    }
    return is;
}

} // namespace eon::tpl::detail
