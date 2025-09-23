#pragma once

#include <ratio>
#include <string_view>

#define EON_CHRONO_GENERATE_TIME_UNITS(name_prefix, string_prefix) \
    template <typename Period> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit() noexcept { \
        return string_prefix ## ""; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::atto>() noexcept { \
        return string_prefix ## "as"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::femto>() noexcept { \
        return string_prefix ## "fs"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::pico>() noexcept { \
        return string_prefix ## "ps"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::nano>() noexcept { \
        return string_prefix ## "ns"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::micro>() noexcept { \
        return string_prefix ## "us"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::milli>() noexcept { \
        return string_prefix ## "ms"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::centi>() noexcept { \
        return string_prefix ## "cs"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::deci>() noexcept { \
        return string_prefix ## "ds"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::ratio<1>>() noexcept { \
        return string_prefix ## "s"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::deca>() noexcept { \
        return string_prefix ## "das"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::hecto>() noexcept { \
        return string_prefix ## "hs"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::kilo>() noexcept { \
        return string_prefix ## "ks"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::mega>() noexcept { \
        return string_prefix ## "Ms"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::giga>() noexcept { \
        return string_prefix ## "Gs"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::tera>() noexcept { \
        return string_prefix ## "Ts"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::peta>() noexcept { \
        return string_prefix ## "Ps"; \
    } \
    \
    template <> \
    [[nodiscard]] consteval std:: name_prefix ## string_view name_prefix ## time_unit<std::exa>() noexcept { \
        return string_prefix ## "Ex"; \
    } \

namespace eon::chrono {

    //
    // time_unit for char
    //

    EON_CHRONO_GENERATE_TIME_UNITS(,)

    //
    // time_unit for wchar_t
    //

    EON_CHRONO_GENERATE_TIME_UNITS(w, L)

    //
    // time_unit for char16_t
    //

    EON_CHRONO_GENERATE_TIME_UNITS(u16, u)

    //
    // time_unit for char32_t
    //

    EON_CHRONO_GENERATE_TIME_UNITS(u32, U)

    //
    // time_unit for char8_t
    //

#ifdef __cpp_lib_char8_t
    EON_CHRONO_GENERATE_TIME_UNITS(u8, u8)
#endif

    //
    // helping functions
    //

    template <typename Period, typename CharT>
    [[nodiscard]] consteval std::basic_string_view<CharT> get_time_unit() noexcept {
        if constexpr (std::same_as<CharT, char>) {
            return time_unit<Period>();
        }
        else if constexpr (std::same_as<CharT, wchar_t>) {
            return wtime_unit<Period>();
        }
        else if constexpr (std::same_as<CharT, char16_t>) {
            return u16time_unit<Period>();
        }
        else if constexpr (std::same_as<CharT, char32_t>) {
            return u32time_unit<Period>();
        }
    #ifdef __cpp_lib_char8_t
        else if constexpr (std::same_as<CharT, char8_t>) {
            return u8time_unit<Period>();
        }
    #endif

        return {};
    }

}
