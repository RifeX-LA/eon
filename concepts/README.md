# Concepts

Include `<eon/concepts.hpp>`. Public concepts are in `eon`; the project targets C++23.

| Group | Concepts |
| --- | --- |
| Qualifiers and identity | `decayed`, `without_cv`, `without_ref`, `without_cvref`, `ref_or_ptr`, `exists_in` |
| Invocation | `indirectable`, `invocable_r`, `invocable_conv_r` |
| Templates | `specialization_of`, `derived_from_specialization_of` |
| Tuples | `tuple_like`, `pair_like` |
| Ranges | `view_of`, `range_of`, `input_range_of`, `forward_range_of`, `bidirectional_range_of`, `random_access_range_of`, `contiguous_range_of` |
| Characters | `character`, `range_of_characters`, `character_ptr`, `character_array`, `bounded_character_array`, `unbounded_character_array` |
| Strings | `string_like`, `sized_string_like`, `unsized_string_like` |

`invocable_r<Fn, R, Args...>` requires exactly `R` as the result type;
`invocable_conv_r` allows conversion to `R`. The `*_range_of<R, V>` concepts
compare the range value type with `V` exactly.

`tuple_like<T>` checks tuple size, element types and unqualified `get<I>` for
mutable and const lvalues, with the expected reference return types. `pair_like`
additionally requires two elements. The template-specialization concepts support
templates whose parameters are types.

```cpp
#include <vector>
#include <eon/concepts.hpp>

static_assert(eon::character<char8_t>);
static_assert(eon::specialization_of<std::vector<int>, std::vector>);
static_assert(eon::contiguous_range_of<std::vector<int>, int>);
```
