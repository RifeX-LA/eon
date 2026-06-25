#pragma once

#include <eon/concepts.hpp>
#include <eon/tpl/detail/io_view.hpp>

namespace eon::tpl {

    /**
     * @brief returns wrapper, that can be used with iostreams
     * @param tuple tuple-like object to wrap
     */
    template <tuple_like Tuple>
    [[nodiscard]] constexpr auto io(Tuple && tuple) noexcept {
        return detail::io_view{tuple, ", "};
    }

    /**
     * @brief returns wrapper, that can be used with iostreams
     * @param tuple tuple-like object to wrap
     * @param sep custom separator between tuple elements
     */
    template <tuple_like Tuple, string_like Sep>
    [[nodiscard]] constexpr auto io(Tuple && tuple, Sep const & sep) noexcept {
        if constexpr (specialization_of<Sep, std::basic_string>) {
            return detail::io_view{tuple, std::basic_string_view(sep)};
        }
        else {
            return detail::io_view{tuple, sep};
        }
    }

}
