#pragma once

#include <eon/concepts.hpp>
#include <eon/tpl/type_traits.hpp>

namespace eon::tpl {

    template <typename Fn, typename Tuple>
    concept tuple_invocable = tuple_like<Tuple> && tuple_all_of_v<std::is_invocable, Fn, Tuple>;

    template <typename Fn, typename Tuple>
    concept tuple_indexes_invocable = eon::detail::has_tuple_size<Tuple> && index_sequence_all_of_v<std::is_invocable, Fn, std::tuple_size_v<Tuple>>;

}
