#pragma once

#include <utility> // std::forward
#include <type_traits>

#ifdef EON_SPARK_INCLUDE_BOOST
#include <boost/lexical_cast.hpp>
#endif

namespace eon::spark {

    template <typename Target>
    struct static_caster_t final {
        template <typename Source>
        [[nodiscard]] constexpr Target operator()(Source && value) const
        noexcept(noexcept(static_cast<Target>(std::forward<Source>(value)))) {
            return static_cast<Target>(std::forward<Source>(value));
        }
    };

    template <typename Target>
    inline constexpr static_caster_t<Target> static_caster;

    inline constexpr static_caster_t<bool> has_value;


    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    struct dynamic_caster_t final {
        template <typename Source>
        [[nodiscard]] Target operator()(Source && value) const noexcept(std::is_pointer_v<Target>) {
            return dynamic_cast<Target>(std::forward<Source>(value));
        }
    };

    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    inline constexpr dynamic_caster_t<Target> dynamic_caster;


    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    struct reinterpret_caster_t final {
        template <typename Source>
        [[nodiscard]] constexpr Target operator()(Source && value) const noexcept {
            return reinterpret_cast<Target>(std::forward<Source>(value));
        }
    };

    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    inline constexpr reinterpret_caster_t<Target> reinterpret_caster;


    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    struct const_caster_t final {
        template <typename Source>
        [[nodiscard]] constexpr Target operator()(Source && value) const noexcept {
            return const_cast<Target>(std::forward<Source>(value));
        }
    };

    template <typename Target>
    requires (std::is_reference_v<Target> || std::is_pointer_v<Target>)
    inline constexpr const_caster_t<Target> const_caster;


#ifdef EON_SPARK_INCLUDE_BOOST

    template <typename Target>
    struct lexical_caster_t final {
        template <typename Source>
        [[nodiscard]] Target operator()(Source && value) const {
            return boost::lexical_cast<Target>(std::forward<Source>(value));
        }

        template <character CharT>
        [[nodiscard]] Target operator()(const CharT * chars, std::size_t count) const {
            return boost::lexical_cast<Target>(chars, count);
        }
    };

    template <typename Target>
    inline constexpr lexical_caster_t<Target> lexical_caster;

#endif

}
