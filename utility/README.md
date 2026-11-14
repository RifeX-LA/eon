# Utility

Include `<eon/utility.hpp>`. Names are in `eon`; the project targets C++23.

- `pass_fn(fn)` accepts an lvalue. It copies trivially copy-constructible/destructible
  callables no larger than a pointer; otherwise it returns `std::ref(fn)`.
- `unwrap_ref(value)` unwraps `std::reference_wrapper`, forwarding other inputs.
- `unwrap_ptr(value)` dereferences raw pointers, forwarding other inputs.
  A pointer must be valid to dereference; smart pointers are not unwrapped.
- `always_false<T>` is a dependent false constant for templates.

`type_pack<T...>` stores types. `make_type_pack_t<T>` extracts a type pack
from a tuple-like type, `std::variant`, or an existing type pack; reference
types are not accepted.

```cpp
#include <tuple>
#include <type_traits>
#include <eon/utility.hpp>

using types = eon::make_type_pack_t<std::tuple<int, double>>;
static_assert(std::same_as<types, eon::type_pack<int, double>>);

int count = 0;
eon::for_each_type_in_pack<types>([&](auto type) { ++count; });
```

`for_each_type<T...>(fn)` and `for_each_type_in_pack<Pack>(fn)` pass
`std::type_identity<T>` objects in declaration order, including duplicates.
The `invocable_for_types` and `invocable_for_packed_types` concepts and
corresponding `is_nothrow_*` traits describe these invocations.
`for_each_tuple_element<Tuple>` passes `std::tuple_element<I, Tuple>` traits;
`for_each_variant_alternative<Variant>` passes
`std::variant_alternative<I, Variant>` traits.
