#pragma once

#include <iostream>

#include <eon/chrono/timer.hpp>
#include <eon/chrono/time_unit.hpp>

namespace eon::chrono {

    enum class writing_time_unit {
        enable,
        disable
    };

    /**
     * @brief std::chrono based auto timer for ostreams of <b>CharT</b> with traits <b>Traits</b>
     * @tparam Rep an arithmetic type representing the number of ticks
     * @tparam Period <b>std::ratio</b> representing the tick period
     */
    template <typename Rep, typename Period, typename CharT, typename Traits = std::char_traits<CharT>, writing_time_unit Opt = writing_time_unit::enable>
    class basic_auto_timer : public timer<Rep, Period> {
    public:
        explicit basic_auto_timer(std::basic_ostream<CharT, Traits> & os): m_os(os) {
            this->reset();
        }

        virtual ~basic_auto_timer() noexcept {
            auto const elapsed_time = this->elapsed();
            m_os << elapsed_time;
            if constexpr (Opt == writing_time_unit::enable) {
                m_os << get_time_unit<Period, CharT>();
            }
        }

        /**
         * @brief returns the stream in which the result will be written
         */
        [[nodiscard]] std::basic_ostream<CharT, Traits> & stream() const noexcept {
            return m_os;
        }

    protected:
        std::basic_ostream<CharT, Traits> & m_os;
    };

    template <typename Rep = double, typename Period = std::ratio<1>, writing_time_unit Opt = writing_time_unit::enable>
    using auto_timer = basic_auto_timer<Rep, Period, char, std::char_traits<char>, Opt>;

    template <typename Rep = double, typename Period = std::ratio<1>, writing_time_unit Opt = writing_time_unit::enable>
    using wauto_timer = basic_auto_timer<Rep, Period, wchar_t, std::char_traits<wchar_t>, Opt>;

    template <typename Rep = double, typename Period = std::ratio<1>, writing_time_unit Opt = writing_time_unit::enable>
    using u16auto_timer = basic_auto_timer<Rep, Period, char16_t, std::char_traits<char16_t>, Opt>;

    template <typename Rep = double, typename Period = std::ratio<1>, writing_time_unit Opt = writing_time_unit::enable>
    using u32auto_timer = basic_auto_timer<Rep, Period, char32_t, std::char_traits<char32_t>, Opt>;

#ifdef __cpp_lib_char8_t
    template <typename Rep = double, typename Period = std::ratio<1>, writing_time_unit Opt = writing_time_unit::enable>
    using u8auto_timer = basic_auto_timer<Rep, Period, char8_t, std::char_traits<char8_t>, Opt>;
#endif

} // namespace eon::chrono
