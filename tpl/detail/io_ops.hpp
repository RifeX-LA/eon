#pragma once

#include <iosfwd>
#include <algorithm>

#include <eon/concepts.hpp>
#include <eon/spark/string.hpp>

namespace eon::tpl::detail {

    template <character CharT, typename Traits>
    [[nodiscard]] bool expect_char(std::basic_istream<CharT, Traits> & is, CharT expected) {
        return Traits::to_char_type(is.get()) == expected;
    }

    template <character CharT, typename Traits>
    bool accept_char(std::basic_istream<CharT, Traits> & is, CharT expected) {
        if (!expect_char(is, expected)) {
            is.unget();
            is.setstate(std::basic_istream<CharT, Traits>::failbit);
            return false;
        }
        return true;
    }

    template <character CharT, typename Traits, input_range_of<CharT> Rng>
    bool accept_range(std::basic_istream<CharT, Traits> & is, Rng && expected_rng) {
        auto putback_on_failure = [&](bidirectional_range_of<CharT> auto && rng) {
            for (CharT const c : rng | std::views::reverse) {
                is.putback(c);
            }
            is.setstate(std::basic_istream<CharT, Traits>::failbit);
        };

        if constexpr (std::ranges::bidirectional_range<Rng>) {
            auto it = std::ranges::find_if(expected_rng, [&](CharT expected) {
                return !expect_char(is, expected);
            });

            if (it != std::ranges::end(expected_rng)) {
                putback_on_failure(std::ranges::subrange(std::ranges::begin(expected_rng), ++it));
                return false;
            }
        }
        else {
            std::basic_string<CharT, Traits> buffer;
            if constexpr (std::ranges::sized_range<Rng>) {
                buffer.reserve(std::ranges::size(expected_rng));
            }

            auto save_to_buffer_and_compare = [&](CharT expected) {
                buffer += expected;
                return expect_char(is, expected);
            };

            if (!std::ranges::all_of(expected_rng, save_to_buffer_and_compare)) {
                putback_on_failure(buffer);
                return false;
            }
        }

        return true;
    }

    template <character CharT, typename Traits>
    bool accept_range(std::basic_istream<CharT, Traits> & is, CharT const * str) {
        return accept_range(is, spark::make_input_str(str));
    }

}
