#pragma once

#include <iosfwd>

#include <eon/concepts.hpp>
#include <eon/tpl/detail/io_ops.hpp>

namespace eon::tpl::detail {

    template <character CharT>
    struct quoted_char final {
        CharT symbol;
        CharT delim;

        constexpr quoted_char(CharT c, CharT delim) noexcept : symbol(c), delim(delim) {}

        quoted_char(quoted_char &&) = delete;
        quoted_char(quoted_char const &) = delete;
    };

    template <typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & os, quoted_char<CharT> view) {
        return os << view.delim << view.symbol << view.delim;
    }

    template <typename CharT, typename Traits>
    std::basic_istream<CharT, Traits> & operator>>(std::basic_istream<CharT, Traits> & is, quoted_char<CharT> view) {
        accept_char(is, view.delim);
        accept_char(is, view.symbol);
        accept_char(is, view.delim);

        return is;
    }

}
