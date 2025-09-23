#pragma once

#include <chrono>
#include <functional>
#include <concepts>

#include <eon/chrono/time_unit.hpp>


namespace eon::chrono {

    /**
     * @brief std::chrono based timer
     * @tparam Rep an arithmetic type representing the number of ticks
     * @tparam Period <b>std::ratio</b> representing the tick period
     */
    template <typename Rep = double, typename Period = std::ratio<1>>
    class timer {
    public:
        using rep_type = Rep;
        using period_type = Period;
        using duration_type = std::chrono::duration<rep_type, period_type>;
        using clock_type = std::chrono::steady_clock;

    public:
        /**
         * @brief creates timer with current time
         */
        timer() : m_start(clock_type::now()) {}

        /**
         * @brief returns elapsed duration
         */
        [[nodiscard]] duration_type duration() const {
            return std::chrono::duration_cast<duration_type>(clock_type::now() - m_start);
        }

        /**
         * @brief returns elapsed time
         */
        [[nodiscard]] rep_type elapsed() const {
            return duration().count();
        }

        /**
         * @brief resets the timer
         */
        void reset() {
            m_start = clock_type::now();
        }

        /**
         * @brief returns the time point which from the timer started counting
         */
        [[nodiscard]] clock_type::time_point time_point() const noexcept {
            return m_start;
        }

        /**
         * @brief returns callable's execution duration
         * @param callable any object that can be invoked with <b>args</b>
         * @param args arguments to pass to <b>callable</b>
         */
        template <typename... Args, std::invocable<Args...> Callable>
        static duration_type duration(Callable && callable, Args &&... args) {
            timer const timer;
            std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
            return timer.duration();
        }

        /**
         * @brief returns pair contains callable's execution duration and callable's invoke result
         * @param callable any object that can be invoked with <b>args</b>
         * @param args arguments to pass to <b>callable</b>
         */
        template <typename... Args, std::invocable<Args...> Callable>
        requires (!std::is_void_v<std::invoke_result_t<Callable, Args...>>)
        static std::pair<duration_type, std::invoke_result_t<Callable, Args...>> duration_r(Callable && callable, Args &&... args) {
            timer const timer;
            auto && result = std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
            auto dur = timer.duration();

            return {std::move(dur), std::forward<decltype(result)>(result)};
        }

        /**
         * @brief returns callable's execution time
         * @param callable any object that can be invoked with <b>args</b>
         * @param args arguments to pass to <b>callable</b>
         */
        template <typename... Args, std::invocable<Args...> Callable>
        static rep_type elapsed(Callable && callable, Args &&... args) {
            return duration(std::forward<Callable>(callable), std::forward<Args>(args)...).count();
        }

        /**
         * @brief returns pair contains callable's execution time and callables' invoke result
         * @param callable any object that can be invoked with <b>args</b>
         * @param args arguments to pass to <b>callable</b>
         */
        template <typename... Args, std::invocable<Args...> Callable>
        requires (!std::is_void_v<std::invoke_result_t<Callable, Args...>>)
        static std::pair<rep_type, std::invoke_result_t<Callable, Args...>> elapsed_r(Callable && callable, Args &&... args) {
            auto && [dur, result] = duration_r(std::forward<Callable>(callable), std::forward<Args>(args)...);
            return {dur.count(), std::forward<decltype(result)>(result)};
        }

        /**
         * @brief executes <b>callable</b> object <b>count</b> times and returns the average execution duration
         * @param count count of iterations
         * @param callable any object that can be invoked with <b>args</b>
         * @param args arguments to pass to <b>callable</b>
         */
        template <typename... Args, std::invocable<Args...> Callable>
        static duration_type avg_duration(std::size_t count, Callable && callable, Args &&... args) {
            duration_type total{};
            for (std::size_t i = 0; i < count; ++i) {
                total += duration(std::forward<Callable>(callable), std::forward<Args>(args)...);
            }

            return static_cast<duration_type>(total / static_cast<rep_type>(count));
        }

        /**
         * @brief executes <b>callable</b> <b>count</b> times and returns the average execution time
         * @param count count of iterations
         * @param callable any object that can be invoked with <b>args</b>
         * @param args arguments to pass to <b>callable</b>
         */
        template <typename... Args, std::invocable<Args...> Callable>
        static rep_type avg_elapsed(std::size_t count, Callable && callable, Args &&... args) {
            return avg_duration(count, std::forward<Callable>(callable), std::forward<Args>(args)...).count();
        }

    private:
        clock_type::time_point m_start;
    };

}
