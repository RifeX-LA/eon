#pragma once

#include <functional>
#include <type_traits>

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

}
