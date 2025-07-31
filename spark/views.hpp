#pragma once

#include <ranges>

#include <eon/spark/cast.hpp>
#include <eon/spark/utility.hpp>
#include <eon/spark/string.hpp>

#ifdef EON_SPARK_INCLUDE_BOOST
#include <boost/hof/construct.hpp>
#endif

namespace eon::spark::views {

    template <typename Target>
    struct static_caster_t final : std::ranges::range_adaptor_closure<static_caster_t<Target>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::static_caster<Target>);
        }
    };

    template <typename Target>
    inline constexpr static_caster_t<Target> static_caster;


    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    struct dynamic_caster_t final : std::ranges::range_adaptor_closure<dynamic_caster_t<Target>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::dynamic_caster<Target>);
        }
    };

    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    inline constexpr dynamic_caster_t<Target> dynamic_caster;


    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    struct reinterpret_caster_t final : std::ranges::range_adaptor_closure<reinterpret_caster_t<Target>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::reinterpret_caster<Target>);
        }
    };

    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    inline constexpr reinterpret_caster_t<Target> reinterpret_caster;


    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    struct const_caster_t final : std::ranges::range_adaptor_closure<dynamic_caster_t<Target>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::const_caster<Target>);
        }
    };

    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    inline constexpr const_caster_t<Target> const_caster;


    struct has_value_t final : std::ranges::range_adaptor_closure<has_value_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::filter(std::forward<Rng>(rng), spark::has_value);
        }
    };

    inline constexpr has_value_t has_value;


    template <typename Target, str_parsing_mode ParsingMode = str_parsing_mode::relaxed>
    requires (std::floating_point<Target> || std::integral<Target>)
    struct from_str_t final : std::ranges::range_adaptor_closure<from_str_t<Target, ParsingMode>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::from_str<Target, ParsingMode>);
        }
    };

    template <typename Target, str_parsing_mode ParsingMode = str_parsing_mode::relaxed>
    requires (std::floating_point<Target> || std::integral<Target>)
    inline constexpr from_str_t<Target, ParsingMode> from_str;


    struct to_str_t final : std::ranges::range_adaptor_closure<to_str_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::to_str);
        }
    };

    inline constexpr to_str_t to_str;


    struct indirect_t final : std::ranges::range_adaptor_closure<indirect_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::indirect);
        }
    };

    inline constexpr indirect_t indirect;


    struct addressof_t final : std::ranges::range_adaptor_closure<addressof_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::addressof);
        }
    };

    inline constexpr addressof_t addressof;


    struct to_underlying_t final : std::ranges::range_adaptor_closure<to_underlying_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::to_underlying);
        }
    };

    inline constexpr to_underlying_t to_underlying;


    struct hash_t final : std::ranges::range_adaptor_closure<hash_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::hash);
        }
    };

    inline constexpr hash_t hash;


    struct front_t final : std::ranges::range_adaptor_closure<front_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::front);
        }
    };

    inline constexpr front_t front;


    struct back_t final : std::ranges::range_adaptor_closure<back_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::back);
        }
    };

    inline constexpr back_t back;


    struct invoke_t final : std::ranges::range_adaptor_closure<invoke_t> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::invoke);
        }
    };

    inline constexpr invoke_t invoke;


#ifdef EON_SPARK_INCLUDE_BOOST
    template <typename Target>
    struct lexical_caster_t final : std::ranges::range_adaptor_closure<lexical_caster_t<Target>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), spark::lexical_caster<Target>);
        }
    };

    template <typename Target>
    inline constexpr lexical_caster_t<Target> lexical_caster;


    template <template <typename...> typename Template>
    struct construct_t final : std::ranges::range_adaptor_closure<construct_t<Template>> {
        template <std::ranges::viewable_range Rng>
        [[nodiscard]] constexpr std::ranges::viewable_range auto operator()(Rng && rng) const noexcept {
            return std::views::transform(std::forward<Rng>(rng), boost::hof::construct<Template>());
        }
    };

    template <template <typename...> typename Template>
    inline constexpr construct_t<Template> construct;
#endif

}

namespace eon {
    namespace views = spark::views;
}
