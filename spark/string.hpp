#pragma once

#include <expected>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <charconv>
#include <string>
#include <array>
#include <limits>

#include <eon/concepts.hpp>

namespace eon::spark {

    enum class str_parsing_mode {
        relaxed,
        strict
    };

    template <typename Target, str_parsing_mode ParsingMode = str_parsing_mode::relaxed>
    requires (std::is_arithmetic_v<Target>)
    struct from_str_t final {
        template <std::ranges::contiguous_range Rng>
        requires (std::same_as<std::ranges::range_value_t<Rng>, char> && std::integral<Target>)
        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(Rng && rng, int base = 10) const noexcept {
            auto const ptr = std::ranges::data(rng);
            return (*this)(ptr, ptr + std::ranges::size(rng), base);
        }

        template <std::ranges::contiguous_range Rng>
        requires (std::same_as<std::ranges::range_value_t<Rng>, char> && std::floating_point<Target>)
        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(Rng && rng, std::chars_format fmt = std::chars_format::general) const noexcept {
            auto const ptr = std::ranges::data(rng);
            return (*this)(ptr, ptr + std::ranges::size(rng), fmt);
        }

        template <std::ranges::input_range Rng>
        requires (std::same_as<std::ranges::range_value_t<Rng>, char> && std::integral<Target>)
        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(Rng && rng, int base = 10) const noexcept {
            std::string const buffer(std::from_range, std::forward<Rng>(rng));
            return (*this)(buffer.c_str(), buffer.c_str() + buffer.size(), base);
        }

        template <std::ranges::input_range Rng>
        requires (std::same_as<std::ranges::range_value_t<Rng>, char> && std::floating_point<Target>)
        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(Rng && rng, std::chars_format fmt = std::chars_format::general) const noexcept {
            std::string const buffer(std::from_range, std::forward<Rng>(rng));
            return (*this)(buffer.c_str(), buffer.c_str() + buffer.size(), fmt);
        }

        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(const char * beg, const char * end, int base = 10) const noexcept
        requires(std::integral<Target>) {
            return convert(beg, end, base);
        }

        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(const char * beg, const char * end,
                                                                            std::chars_format fmt = std::chars_format::general) const noexcept
        requires(std::floating_point<Target>) {
            return convert(beg, end, fmt);
        }

    private:
        template <exists_in<int, std::chars_format> ParamT>
        [[nodiscard]] constexpr std::expected<Target, std::errc> convert(const char * beg, const char * end, ParamT param) const noexcept {
            auto trimmed_beg = std::ranges::find_if_not(beg, end, [](char c){ return std::isspace(c); });
            if (trimmed_beg != end && *trimmed_beg == '+') {
                ++trimmed_beg;
            }

            Target value;
            auto const [ptr, ec] = std::from_chars(trimmed_beg, end, value, param);
            if (ec == std::errc()) {
                if constexpr (ParsingMode == str_parsing_mode::strict) {
                    if (ptr != end) {
                        return std::unexpected(std::errc::invalid_argument);
                    }
                }
                return value;
            }
            return std::unexpected(ec);
        }
    };

    template <typename Target, str_parsing_mode ParsingMode = str_parsing_mode::relaxed>
    requires (std::is_arithmetic_v<Target>)
    inline constexpr from_str_t<Target, ParsingMode> from_str;


    struct to_str_t final {
        template <std::integral T>
        [[nodiscard]] std::string operator()(T value, int base = 10) const {
            std::array<char, std::numeric_limits<T>::digits + 1> buffer;
            auto const result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, base);
            return result.ec == std::errc() ? std::string(buffer.data(), result.ptr) : std::string();
        }

        template <std::floating_point T>
        [[nodiscard]] std::string operator()(T value, std::chars_format fmt = std::chars_format::general) const {
            std::array<char, std::numeric_limits<T>::max_exponent10 + 1> buffer;
            auto const result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, fmt);
            return result.ec == std::errc() ? std::string(buffer.data(), result.ptr) : std::string();
        }
    };

    inline constexpr to_str_t to_str;

}
