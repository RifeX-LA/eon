#pragma once

#include <functional>
#include <type_traits>

#include <eon/concepts.hpp>

namespace eon {

    template <typename T>
    inline constexpr bool always_false = false;


    /**
     * @brief returns argument by copy if it's cheaper to pass <b>Fn</b> by copy,
     * returns std::reference_wrapper<Fn> otherwise,
     * i.e., provides a way of cheap copying of an argument
     */
    template <typename Fn>
    [[nodiscard]] constexpr auto pass_fn(Fn & fn) noexcept {
        constexpr bool pass_by_value = sizeof(Fn) <= sizeof(void*)
                                            && std::is_trivially_copy_constructible_v<Fn>
                                            && std::is_trivially_destructible_v<Fn>;

        if constexpr (pass_by_value) {
            return fn;
        }
        else {
            return std::ref(fn);
        }
    }

    /**
     * @return stored value, if <b>T</b> is a specialization of <b>std::reference_wrapper</b>
     * @return <b>std::forward<T>(value)</b> otherwise
     */
    template <typename T>
    [[nodiscard]] constexpr decltype(auto) unwrap_ref(T && value) noexcept {
        if constexpr (specialization_of<T, std::reference_wrapper>) {
            return value.get();
        }
        else {
            return std::forward<T>(value);
        }
    }

    /**
     * @return stored value, if <b>T</b> is a <i>pointer type</i>
     * @return <b>std::forward<T>(value)</b> otherwise
     */
    template <typename T>
    [[nodiscard]] constexpr decltype(auto) unwrap_ptr(T && value) noexcept {
        if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>) {
            return *value;
        }
        else {
            return std::forward<T>(value);
        }
    }

}
