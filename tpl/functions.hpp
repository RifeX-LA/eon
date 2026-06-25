#pragma once

#include <utility>
#include <functional>

#include <eon/utility.hpp>
#include <eon/tpl/concepts.hpp>
#include <eon/tpl/type_traits.hpp>

namespace eon::tpl {

    namespace detail {

        template <typename T>
        struct template_type_invoker final {};

        template <template <typename...> typename Template, typename... Types>
        struct template_type_invoker<Template<Types...>> {
            template <typename Fn>
            constexpr void operator()(Fn fn) const {
                (std::invoke(fn, std::type_identity<Types>{}), ...);
            }
        };

    }

    /**
     * @brief calls callable object <b>fn</b> for each element of tuple-like object <b>tuple</b>
    */
    template <typename Tuple, tuple_invocable<Tuple> Fn>
    constexpr void for_each(Tuple && tuple, Fn fn) noexcept(is_nothrow_tuple_invocable_v<Fn, Tuple>) {
        [&tuple, fn = eon::pass_fn(fn)]<std::size_t... I>(std::index_sequence<I...>) {
                (std::invoke(fn, std::get<I>(std::forward<Tuple>(tuple))), ...);
        }(make_tuple_index_sequence<std::remove_cvref_t<Tuple>>{});
    }

    template <typename Tuple, tuple_indexes_invocable<Tuple> Fn>
    constexpr void for_each_tuple_index(Fn fn) noexcept(is_nothrow_tuple_indexes_invocable_v<Fn, Tuple>) {
        [fn = eon::pass_fn(fn)]<std::size_t... I>(std::index_sequence<I...>) {
            (std::invoke(fn, std::integral_constant<std::size_t, I>{}), ...);
        }(make_tuple_index_sequence<std::remove_cvref_t<Tuple>>{});
    }

    template <typename T, typename Fn>
    constexpr void for_each_template_type(Fn fn) noexcept(noexcept(detail::template_type_invoker<T>{}(eon::pass_fn(fn)))) {
        detail::template_type_invoker<T>{}(eon::pass_fn(fn));
    }

}
