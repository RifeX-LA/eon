#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace eon::tpl {

    template <typename Tuple>
    using make_tuple_index_sequence = std::make_index_sequence<std::tuple_size_v<Tuple>>;


    namespace detail {

        template <template <typename...> typename Trait, typename Fn, typename Tuple, typename Indices>
        struct tuple_all_of_impl {};

        template <template <typename...> typename Trait, typename Fn, typename Tuple, std::size_t... Is>
        struct tuple_all_of_impl<Trait, Fn, Tuple, std::index_sequence<Is...>> :
            std::bool_constant<(Trait<Fn, decltype(std::get<Is>(std::declval<Tuple&&>()))>::value && ...)> {};


        template <template <typename...> typename Trait, typename Fn, typename Indices>
        struct index_sequence_all_of_impl {};

        template <template <typename...> typename Trait, typename Fn, std::size_t... Is>
        struct index_sequence_all_of_impl<Trait, Fn, std::index_sequence<Is...>> :
            std::bool_constant<(Trait<Fn, std::integral_constant<std::size_t, Is>>::value && ...)> {};
    }

    template <template <typename...> typename Trait, typename Fn, typename Tuple>
    struct tuple_all_of : detail::tuple_all_of_impl<Trait, Fn, Tuple, make_tuple_index_sequence<std::remove_cvref_t<Tuple>>> {};

    template <template <typename...> typename Trait, typename Fn, typename Tuple>
    inline constexpr bool tuple_all_of_v = tuple_all_of<Trait, Fn, Tuple>::value;


    template <template <typename...> typename Trait, typename Fn, std::size_t N>
    struct index_sequence_all_of : detail::index_sequence_all_of_impl<Trait, Fn, std::make_index_sequence<N>> {};

    template <template <typename...> typename Trait, typename Fn, std::size_t N>
    inline constexpr bool index_sequence_all_of_v = index_sequence_all_of<Trait, Fn, N>::value;


    template <typename Fn, typename Tuple>
    struct is_nothrow_tuple_invocable : tuple_all_of<std::is_nothrow_invocable, Fn, Tuple> {};

    template <typename Fn, typename Tuple>
    inline constexpr bool is_nothrow_tuple_invocable_v = is_nothrow_tuple_invocable<Fn, Tuple>::value;


    template <typename Fn, std::size_t N>
    struct is_nothrow_index_sequence_invocable : index_sequence_all_of<std::is_nothrow_invocable, Fn, N> {};

    template <typename Fn, std::size_t N>
    inline constexpr bool is_nothrow_index_sequence_invocable_v = is_nothrow_index_sequence_invocable<Fn, N>::value;

    template <typename Fn, typename Tuple>
    struct is_nothrow_tuple_indexes_invocable : is_nothrow_index_sequence_invocable<Fn, std::tuple_size_v<Tuple>> {};

    template <typename Fn, typename Tuple>
    inline constexpr bool is_nothrow_tuple_indexes_invocable_v = is_nothrow_tuple_indexes_invocable<Fn, Tuple>::value;

}
