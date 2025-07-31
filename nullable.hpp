#pragma once

#include <concepts>
#include <optional>

namespace eon {

    template <typename T>
    concept nullable = std::is_default_constructible_v<std::remove_reference_t<T>> &&
        requires (T value) {
            static_cast<bool>(value);
            *value;
        };


    template <nullable Nullable>
    struct nullable_reference : std::type_identity<decltype(*std::declval<Nullable>())> {};

    template <nullable Nullable>
    using nullable_reference_t = typename nullable_reference<Nullable>::type;


    template <nullable Nullable>
    struct nullable_value : std::remove_reference<nullable_reference_t<Nullable>> {};

    template <nullable Nullable>
    using nullable_value_t = typename nullable_value<Nullable>::type;


    namespace detail {

        template <typename Result, nullable Nullable, typename... Args, std::invocable<nullable_reference_t<Nullable>, Args...> Callable>
        requires (nullable<Result> || std::is_void_v<Result>)
        Result make_nullable(Callable && callable, Nullable && value, Args &&... args) {
            if (static_cast<bool>(value)) {
                return Result(std::invoke(std::forward<Callable>(callable), *std::forward<Nullable>(value), std::forward<Args>(args)...));
            }
            return Result{};
        }

    }

    /**
    * @brief The main purpose of the function is to avoid double checks for the case
    * when the value of the function or field, which is a <i>nullable</i> object,
    * must be obtained from a <i>nullable</i> object
    * @return <b>decay_t<invoke_result_t></b> if <i>callable</i> returns <i>nullable</i> object
    * @return <b>std::optional<decay_t<invoke_result_t>></b> otherwise (wrap a common value with the <i>nullable</i> object)
    */
    template <nullable Nullable, typename... Args, std::invocable<nullable_reference_t<Nullable>, Args...> Callable>
    constexpr auto invoke_nullable(Callable && callable, Nullable && value, Args &&... args) {
        using decayed_result_t = std::decay_t<std::invoke_result_t<Callable, nullable_reference_t<Nullable>, Args...>>;

        if constexpr (nullable<decayed_result_t> || std::is_void_v<decayed_result_t>) {
            return detail::make_nullable<decayed_result_t>(std::forward<Callable>(callable), std::forward<Nullable>(value), std::forward<Args>(args)...);
        }
        else {
            return detail::make_nullable<std::optional<decayed_result_t>>(std::forward<Callable>(callable), std::forward<Nullable>(value), std::forward<Args>(args)...);
        }
    }

    /**
    * @brief Unlike <i>invoke_nullable()</i> function, returns the pointer if <i>callable</i> returns lvalue-reference.
    * Thus, it allows <b>avoiding creating a copy</b> of the return value
    * @return <b>add_pointer_t<nullable_value_t<invoke_result_t>></b>, if <i>callable</i> returns lvalue-reference on a <i>nullable</i> object
    * @return <b>add_pointer_t<invoke_result_t></b>, if <i>callable</i> returns lvalue-reference on a <i>non-nullable</i> object
    * @return <b>invoke_nullable(...)</b> otherwise
    */
    template <nullable Nullable, typename... Args, std::invocable<nullable_reference_t<Nullable>, Args...> Callable>
    constexpr auto invoke_nullable_ref(Callable && callable, Nullable && value, Args &&... args) {
        using result_t = std::invoke_result_t<Callable, nullable_reference_t<Nullable>, Args...>;

        if constexpr (std::is_lvalue_reference_v<result_t>) {
            if constexpr (nullable<result_t>) {
                static_assert(std::is_lvalue_reference_v<nullable_reference_t<result_t>>);
                using pointer_t = std::add_pointer_t<nullable_reference_t<result_t>>;

                auto get_address = []<typename T>(T && t) { return std::addressof(std::forward<T>(t)); };
                auto get_result_value_address = [&]<typename... Params>(Params &&... params) {
                    return detail::make_nullable<pointer_t>(get_address, std::invoke(std::forward<Callable>(callable), std::forward<Params>(params)...));
                };
                return detail::make_nullable<pointer_t>(get_result_value_address, std::forward<Nullable>(value), std::forward<Args>(args)...);
            }
            else {
                using pointer_t = std::add_pointer_t<result_t>;

                auto get_result_address = [&]<typename... Params>(Params &&... params) {
                    return std::addressof(std::invoke(std::forward<Callable>(callable), std::forward<Params>(params)...));
                };
                return detail::make_nullable<pointer_t>(get_result_address, std::forward<Nullable>(value), std::forward<Args>(args)...);
            }
        }
        else {
            return eon::invoke_nullable(std::forward<Callable>(callable), std::forward<Nullable>(value), std::forward<Args>(args)...);
        }
    }

}
