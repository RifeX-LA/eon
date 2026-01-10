#pragma once

#include <utility>
#include <tuple>
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


    template <typename T>
    concept ref_or_ptr = std::is_reference_v<T> || std::is_pointer_v<T>;


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


    namespace detail {

        template <typename T>
        concept has_tuple_size = requires { typename std::tuple_size<std::remove_cvref_t<T>>::type; };

        template <std::size_t I, typename U>
        concept tuple_like_for_index = requires (U & u, U const & cu) {
            typename std::tuple_element<I, U>::type;
            { get<I>(u) }  -> std::same_as<std::tuple_element_t<I, U> &>;
            { get<I>(cu) } -> std::same_as<std::tuple_element_t<I, U> const &>;
        };

        template <typename T>
        [[nodiscard]] consteval bool is_tuple_like_impl() noexcept {
            auto is_tuple_like = []<std::size_t... I>(std::index_sequence<I...>) {
                return (tuple_like_for_index<I, T> && ...);
            };

            return is_tuple_like(std::make_index_sequence<std::tuple_size_v<T>>{});
        }

    }

    template <typename T>
    concept tuple_like = detail::has_tuple_size<T> && detail::is_tuple_like_impl<std::remove_cvref_t<T>>();

    template <typename T>
    concept pair_like = tuple_like<T> && std::tuple_size_v<std::remove_cvref_t<T>> == 2;


    template <typename T, typename U>
    concept view_of = std::ranges::view<T> && std::same_as<std::ranges::range_value_t<T>, U>;

    template <typename T, typename U>
    concept range_of = std::ranges::range<T> && std::same_as<std::ranges::range_value_t<T>, U>;

    template <typename T, typename U>
    concept input_range_of = std::ranges::input_range<T> && range_of<T, U>;

    template <typename T, typename U>
    concept forward_range_of = std::ranges::forward_range<T> && range_of<T, U>;

    template <typename T, typename U>
    concept bidirectional_range_of = std::ranges::bidirectional_range<T> && range_of<T, U>;

    template <typename T, typename U>
    concept random_access_range_of = std::ranges::random_access_range<T> && range_of<T, U>;

    template <typename T, typename U>
    concept contiguous_range_of = std::ranges::contiguous_range<T> && range_of<T, U>;

    template <typename T>
    concept range_of_characters = std::ranges::range<T> && character<std::ranges::range_value_t<T>>;


    template <typename T>
    concept character_ptr = std::is_pointer_v<std::remove_reference_t<T>> && character<std::remove_pointer_t<std::remove_reference_t<T>>>;

    template <typename T>
    concept character_array = std::is_array_v<std::remove_reference_t<T>> && character<std::remove_extent_t<std::remove_reference_t<T>>>;

    template <typename T>
    concept bounded_character_array = std::is_bounded_array_v<std::remove_reference_t<T>> && character<std::remove_extent_t<std::remove_reference_t<T>>>;

    template <typename T>
    concept unbounded_character_array = std::is_unbounded_array_v<std::remove_reference_t<T>> && character<std::remove_extent_t<std::remove_reference_t<T>>>;


    template <typename T>
    concept string_like = specialization_of<T, std::basic_string> || specialization_of<T, std::basic_string_view> ||
                          character_ptr<T> || character_array<T>;

    template <typename T>
    concept sized_string_like = specialization_of<T, std::basic_string> || specialization_of<T, std::basic_string_view> ||
                                  bounded_character_array<T>;

    template <typename T>
    concept unsized_string_like = character_ptr<T> || unbounded_character_array<T>;


    namespace detail {

        template <typename StrategyT>
        struct choice_t {
            StrategyT strategy = StrategyT{};
            bool nothrow = false;
        };

    }

}
