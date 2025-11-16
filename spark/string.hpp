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

    namespace detail {

        template <typename T>
        concept conversion_param = exists_in<T, int, std::chars_format>;

    }

    enum class str_parsing {
        relaxed,
        strict
    };

    template <typename Target, str_parsing ParsingMode = str_parsing::relaxed>
    requires (std::is_arithmetic_v<Target>)
    struct from_str_t final {
        template <std::ranges::input_range Rng>
        requires (std::same_as<std::ranges::range_value_t<Rng>, char> && std::integral<Target>)
        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(Rng && rng, int base = 10) const
        noexcept(noexcept(convert(std::forward<Rng>(rng), base))) {
            return convert(std::forward<Rng>(rng), base);
        }

        template <std::ranges::input_range Rng>
        requires (std::same_as<std::ranges::range_value_t<Rng>, char> && std::floating_point<Target>)
        [[nodiscard]] constexpr std::expected<Target, std::errc> operator()(Rng && rng, std::chars_format fmt = std::chars_format::general) const
        noexcept(noexcept(convert(std::forward<Rng>(rng), fmt))) {
            return convert(std::forward<Rng>(rng), fmt);
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
        template <std::ranges::input_range Rng>
        [[nodiscard]] static consteval bool can_noexcept_convert() noexcept {
            if constexpr (std::ranges::contiguous_range<Rng>) {
                return noexcept(std::ranges::data(std::declval<Rng&>()) + std::ranges::size(std::declval<Rng&>()));
            }
            else {
                return false;
            }
        }

        template <std::ranges::input_range Rng, detail::conversion_param ParamT>
        [[nodiscard]] constexpr std::expected<Target, std::errc> convert(Rng && rng, ParamT param) const noexcept(can_noexcept_convert<Rng>()) {
            if constexpr (std::ranges::contiguous_range<Rng>) {
                auto const ptr = std::ranges::data(rng);
                return convert(ptr, ptr + std::ranges::size(rng), param);
            }
            else {
                std::string const buffer(std::from_range, std::forward<Rng>(rng));
                return convert(buffer.data(), buffer.data() + buffer.size(), param);
            }
        }

        template <detail::conversion_param ParamT>
        [[nodiscard]] constexpr std::expected<Target, std::errc> convert(const char * beg, const char * end, ParamT param) const noexcept {
            auto trimmed_beg = std::ranges::find_if_not(beg, end, [](char c){ return std::isspace(c); });
            if (trimmed_beg != end && *trimmed_beg == '+') {
                ++trimmed_beg;
            }

            Target value;
            auto const [ptr, ec] = std::from_chars(trimmed_beg, end, value, param);
            if (ec == std::errc()) {
                if constexpr (ParsingMode == str_parsing::strict) {
                    if (ptr != end) {
                        return std::unexpected(std::errc::invalid_argument);
                    }
                }
                return value;
            }
            return std::unexpected(ec);
        }
    };

    template <typename Target, str_parsing ParsingMode = str_parsing::relaxed>
    requires (std::is_arithmetic_v<Target>)
    inline constexpr from_str_t<Target, ParsingMode> from_str;


    struct to_str_t final {
        template <std::integral T>
        [[nodiscard]] std::string operator()(T value, int base = 10) const {
            return convert(value, base);
        }

        template <std::floating_point T>
        [[nodiscard]] std::string operator()(T value, std::chars_format fmt = std::chars_format::general) const {
            return convert(value, fmt);
        }

        template <std::integral T, std::weakly_incrementable OutIt>
        constexpr OutIt operator()(T value, OutIt output, int base = 10) const {
            return convert(value, std::move(output), base);
        }

        template <std::floating_point T, std::weakly_incrementable OutIt>
        constexpr OutIt operator()(T value, OutIt output, std::chars_format fmt = std::chars_format::general) const {
            return convert(value, std::move(output), fmt);
        }

    private:
        template <typename T>
        requires (std::is_arithmetic_v<T>)
        [[nodiscard]] static consteval std::size_t get_buffer_size() noexcept {
            if constexpr (std::integral<T>) {
                return std::numeric_limits<T>::digits + 2;
            }
            else {
                return std::numeric_limits<T>::max_exponent10 + 50;
            }
        }

        template <typename T, std::weakly_incrementable OutIt, detail::conversion_param ParamT>
        requires (std::is_arithmetic_v<T>)
        constexpr OutIt convert(T value, OutIt output, ParamT param) const {
            std::array<char, get_buffer_size<T>()> buffer;
            auto const [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, param);
            if (ec == std::errc()) {
                return std::ranges::copy(buffer.data(), ptr, std::move(output)).out;
            }
            return output;
        }

        template <typename T, detail::conversion_param ParamT>
        requires (std::is_arithmetic_v<T>)
        [[nodiscard]] std::string convert(T value, ParamT param) const {
            std::array<char, get_buffer_size<T>()> buffer;
            auto const [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, param);
            return ec == std::errc() ? std::string(buffer.data(), ptr) : std::string();
        }
    };

    inline constexpr to_str_t to_str;

}
