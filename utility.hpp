#pragma once

#include <functional>
#include <type_traits>
#include <variant>

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
    constexpr bool pass_by_value = sizeof(Fn) <= sizeof(void *) && std::is_trivially_copy_constructible_v<Fn> &&
                                   std::is_trivially_destructible_v<Fn>;

    if constexpr (pass_by_value) {
        return fn;
    } else {
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
    } else {
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
    } else {
        return std::forward<T>(value);
    }
}

template <typename...>
struct type_pack {};

namespace detail {
template <typename TupleLike, typename IndexSeq>
struct make_tuple_type_pack {};

template <typename TupleLike, std::size_t... I>
struct make_tuple_type_pack<TupleLike, std::index_sequence<I...>>
    : std::type_identity<type_pack<std::tuple_element_t<I, TupleLike>...>> {};

template <typename VariantT, typename IndexSeq>
struct make_variant_type_pack {};

template <typename VariantT, std::size_t... I>
struct make_variant_type_pack<VariantT, std::index_sequence<I...>>
    : std::type_identity<type_pack<std::variant_alternative_t<I, VariantT>...>> {};

template <typename T, bool = tuple_like<T>, bool = specialization_of<T, std::variant>>
struct make_type_pack_impl : std::type_identity<std::remove_cv_t<T>> {};

template <typename T>
struct make_type_pack_impl<T, true, false> : make_tuple_type_pack<T, std::make_index_sequence<std::tuple_size_v<T>>> {};

template <typename T>
struct make_type_pack_impl<T, false, true>
    : make_variant_type_pack<T, std::make_index_sequence<std::variant_size_v<T>>> {};
} // namespace detail

template <without_ref T>
requires (tuple_like<T> || specialization_of<T, std::variant> || specialization_of<T, type_pack>)
struct make_type_pack : detail::make_type_pack_impl<T>::type {
    using type = typename detail::make_type_pack_impl<T>::type;
};

template <without_ref T>
using make_type_pack_t = typename make_type_pack<T>::type;

template <typename Fn, typename... Types>
concept invocable_for_types = (std::invocable<Fn, std::type_identity<Types>> && ...);

namespace detail {
template <typename Fn, typename Pack>
struct invocable_for_type_pack : std::false_type {};

template <typename Fn, typename... Types>
struct invocable_for_type_pack<Fn, type_pack<Types...>> : std::bool_constant<invocable_for_types<Fn, Types...>> {};
} // namespace detail

template <typename Fn, typename T>
concept invocable_for_packed_types =
    requires { typename make_type_pack<T>::type; } && detail::invocable_for_type_pack<Fn, make_type_pack_t<T>>::value;

template <typename Fn, typename... Types>
struct is_nothrow_invocable_for_types
    : std::bool_constant<(std::is_nothrow_invocable_v<Fn, std::type_identity<Types>> && ...)> {};

template <typename Fn, typename... Types>
constexpr inline bool is_nothrow_invocable_for_types_v = is_nothrow_invocable_for_types<Fn, Types...>::value;

namespace detail {
template <typename Fn, typename Pack>
struct is_nothrow_invocable_for_packed_types : std::false_type {};

template <typename Fn, typename... Types>
struct is_nothrow_invocable_for_packed_types<Fn, type_pack<Types...>> : is_nothrow_invocable_for_types<Fn, Types...> {};
} // namespace detail

template <typename Fn, typename T>
struct is_nothrow_invocable_for_packed_types : detail::is_nothrow_invocable_for_packed_types<Fn, make_type_pack_t<T>> {
};

template <typename Fn, typename T>
constexpr inline bool is_nothrow_invocable_for_packed_types_v = is_nothrow_invocable_for_packed_types<Fn, T>::value;

template <typename... Types, invocable_for_types<Types...> Fn>
constexpr void for_each_type(Fn fn) noexcept(is_nothrow_invocable_for_types_v<Fn, Types...>) {
    (fn(std::type_identity<Types>{}), ...);
}

template <without_ref T, invocable_for_packed_types<T> Fn>
constexpr void for_each_type_in_pack(Fn fn) noexcept(is_nothrow_invocable_for_packed_types_v<Fn, T>) {
    [fn = pass_fn(fn)]<typename... Types>(type_pack<Types...>) { for_each_type<Types...>(fn); }(make_type_pack_t<T>{});
}

template <without_ref T, typename Fn>
requires (tuple_like<T>)
constexpr void for_each_tuple_element(Fn fn) {
    [fn = pass_fn(fn)]<std::size_t... I>(std::index_sequence<I...>) {
        (fn(std::tuple_element<I, T>{}), ...);
    }(std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <without_ref T, typename Fn>
requires (specialization_of<T, std::variant>)
constexpr void for_each_variant_alternative(Fn fn) {
    [fn = pass_fn(fn)]<std::size_t... I>(std::index_sequence<I...>) {
        (fn(std::variant_alternative<I, T>{}), ...);
    }(std::make_index_sequence<std::variant_size_v<T>>{});
}

} // namespace eon
