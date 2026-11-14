# Nullable invocation

Include `<eon/nullable.hpp>`. Names are in `eon`; the project targets C++23.

`nullable<T>` requires a default-constructible underlying type, a conversion
usable with `static_cast<bool>`, and dereferencing. Examples include `std::optional`,
raw pointers and owning smart pointers.

`invoke_nullable(callable, value, args...)` calls
`std::invoke(callable, *value, args...)` only when `value` is present.
A nullable invocation result is returned directly as a decayed value;
a non-nullable result is wrapped in `std::optional`. Void callbacks are
supported. An absent input returns an empty/default result without invoking
the callable.

```cpp
#include <optional>
#include <eon/nullable.hpp>

struct widget {
    std::optional<int> number;
};

std::optional<widget> object{widget{42}};
auto copy = eon::invoke_nullable(&widget::number, object); // optional<int>
auto pointer = eon::invoke_nullable_ref(&widget::number, object); // int*
if (pointer) {
    *pointer = 7; // changes object->number
}
```

`invoke_nullable_ref` returns a pointer for lvalue-reference results. If that
reference names a nullable object, the pointer refers to its contained value
only when present. For other result categories it delegates to
`invoke_nullable`. Returned pointers do not extend object lifetimes.
Callable exceptions propagate.

`nullable_reference_t<T>` describes dereferencing `T`;
`nullable_value_t<T>` removes the reference from that type.
