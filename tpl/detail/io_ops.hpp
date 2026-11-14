#pragma once

#include <algorithm>
#include <istream>

#include <eon/concepts.hpp>
#include <eon/spark/string.hpp>

namespace eon::tpl::detail {

template <character CharT, typename Traits>
[[nodiscard]] bool expect_char(std::basic_istream<CharT, Traits> & is, CharT expected) {
    auto const value = is.get();
    return !Traits::eq_int_type(value, Traits::eof()) && Traits::to_char_type(value) == expected;
}

template <character CharT, typename Traits>
bool accept_char(std::basic_istream<CharT, Traits> & is, CharT expected) {
    auto const value = is.peek();
    if (Traits::eq_int_type(value, Traits::eof()) || Traits::to_char_type(value) != expected) {
        is.setstate(std::basic_istream<CharT, Traits>::failbit);
        return false;
    }
    is.get();
    return true;
}

template <character CharT, typename Traits, std::ranges::input_range Rng>
requires character<std::ranges::range_value_t<Rng>>
bool accept_range(std::basic_istream<CharT, Traits> & is, Rng && expected_rng) {
    if (!is) {
        return false;
    }
    auto * streambuf = is.rdbuf();
    using pos_type = typename Traits::pos_type;
    auto const invalid_pos = pos_type(typename Traits::off_type(-1));
    auto const start = streambuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
    std::basic_string<CharT, Traits> consumed;
    if constexpr (std::ranges::sized_range<Rng>) {
        consumed.reserve(std::ranges::size(expected_rng));
    }
    for (auto symbol : expected_rng) {
        CharT const expected = static_cast<CharT>(symbol);
        // Delay setting failbit until rollback is complete, including when
        // the caller enabled exceptions for failbit/eofbit.
        auto const value = streambuf->sbumpc();
        bool const eof = Traits::eq_int_type(value, Traits::eof());
        if (!eof) {
            consumed.push_back(Traits::to_char_type(value));
        }
        if (eof || consumed.back() != expected) {
            bool restored = start != invalid_pos && streambuf->pubseekpos(start, std::ios_base::in) != invalid_pos;
            if (!restored) {
                restored = true;
                for (CharT actual : consumed | std::views::reverse) {
                    if (Traits::eq_int_type(streambuf->sputbackc(actual), Traits::eof())) {
                        restored = false;
                        break;
                    }
                }
            }
            // EOF from the failed attempt must not hide restored characters.
            auto state = is.rdstate() & std::ios_base::badbit;
            if (!restored) {
                state |= std::ios_base::badbit;
            }
            is.clear(state | std::ios_base::failbit);
            return false;
        }
    }
    return true;
}

template <character CharT, typename Traits, character SepChar>
bool accept_range(std::basic_istream<CharT, Traits> & is, SepChar const * str) {
    return accept_range(is, spark::make_input_str(str));
}

} // namespace eon::tpl::detail
