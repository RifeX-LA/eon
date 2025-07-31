#pragma once

#include <concepts>
#include <type_traits>

namespace eon {

    template <typename T>
    concept decayed = std::same_as<T, std::decay_t<T>>;

    template <typename T>
    concept without_cv = std::same_as<T, std::remove_cv_t<T>>;

    template <typename T>
    concept without_ref = std::same_as<T, std::remove_reference_t<T>>;

    template <typename T>
    concept without_cvref = std::same_as<T, std::remove_cvref_t<T>>;


    template <typename T, typename... Types>
    concept exists_in = std::disjunction_v<std::is_same<T, Types>...>;


    template <typename T>
    concept character = exists_in<std::remove_cvref_t<T>, char, unsigned char, signed char, wchar_t, char8_t, char16_t, char32_t>;


    template <typename T>
    concept indirectable = requires (T value) {
        *value;
    };


    template <typename Fn, typename R, typename... Args>
    concept invocable_r = std::invocable<Fn, Args...> && std::same_as<std::invoke_result_t<Fn, Args...>, R>;

    template <typename Fn, typename R, typename... Args>
    concept invocable_conv_r = std::invocable<Fn, Args...> && std::convertible_to<std::invoke_result_t<Fn, Args...>, R>;


    namespace detail {
        template <typename T, template<typename...> typename Template>
        struct is_specialization_of : std::false_type {};

        template <template<typename...> typename Template, typename... Types>
        struct is_specialization_of<Template<Types...>, Template> : std::true_type {};
    }

    template <typename T, template<typename...> typename Template>
    concept specialization_of = detail::is_specialization_of<std::remove_cvref_t<T>, Template>::value;


    namespace detail {
        template <template <typename...> typename Template, typename... Args>
        void derived_from_specialization_impl(Template<Args...> const &);
    }

    template <class T, template <typename...> typename Template>
    concept derived_from_specialization_of = requires(T const & obj) {
        detail::derived_from_specialization_impl<Template>(obj); // qualified: avoid ADL, handle incomplete types
    };

}
