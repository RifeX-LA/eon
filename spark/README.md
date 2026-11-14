# Spark

Include `<eon/spark.hpp>` or an individual header from this directory.
Function objects are in `eon::spark`; range adaptors are in
`eon::spark::views`, also aliased as `eon::views`. C++23 is required.

## Function objects and views

The module provides cast objects (`static_caster<T>`, `dynamic_caster<T>`,
`const_caster<T>`, `reinterpret_caster<T>`), `get<I>`, `key`, `value`,
`front`, `back`, `indirect`, `addressof`, `hash`, `to_underlying`,
`invoke`, `range_from_nullable` and `has_value`. Many have corresponding
range adaptors.

```cpp
#include <vector>
#include <eon/spark.hpp>

std::vector<int> numbers{1, 2, 3};
auto doubles = numbers | eon::views::static_caster<double>;
auto increment = eon::spark::add(1);
int result = increment(41); // 42
```

Arithmetic factories such as `add(rhs)`, `sub(rhs)`, `divides(rhs)` and
`lshift(rhs)` bind the right operand. They retain the underlying operators'
preconditions: signed overflow, division by zero and invalid shifts are not
checked. `assign(op)` assigns the result of `op` back to its argument.
`proj(projection, fn)` applies a projection to each argument before calling
`fn`. `always(value)` ignores arguments and returns its stored value;
`stub` ignores arguments and returns void.

## Comparisons

`equal`, `not_equal`, `less`, `less_equal`, `greater` and `greater_equal`
are comparator types. Integer comparisons handle mixed signedness.
Floating equality uses an epsilon-scaled tolerance, with absolute tolerance
near zero and relative tolerance for larger magnitudes. Equal infinities compare
equal; NaNs compare unequal and fail ordered comparisons. Approximate comparison
does not define the strict weak ordering required by sorting and ordered
containers; use exact standard comparators for those operations.

Bound predicates include `eq(rhs)`, `neq(rhs)`, `lt(rhs)`, `le(rhs)`,
`gt(rhs)`, `ge(rhs)`, `div(rhs)` and `ndiv(rhs)`; the latter two test
divisibility, not division.

## String conversion

`from_str<T>` returns `std::expected<T, std::errc>` for supported
`std::from_chars` numeric types. It skips leading whitespace and accepts a
single leading plus. Relaxed parsing accepts a valid numeric prefix;
`str_parsing::strict` requires consuming the entire remaining input, including
rejecting trailing whitespace. Integral bases must be in [2, 36].

```cpp
#include <string_view>
#include <eon/spark.hpp>

auto number = eon::spark::from_str<int>(std::string_view{" +42"});
auto text = eon::spark::to_str(42); // std::string{"42"}
```

`to_str` also accepts an output iterator. `make_input_str` creates a view that
stops at the first zero character.

Views are lazy. In particular, `indirect` does not extend the lifetime of
dereferenced objects: keep optional/expected wrappers alive instead of
dereferencing temporary wrappers produced by a transform.

Define `EON_SPARK_INCLUDE_BOOST` before inclusion to enable
`lexical_caster<T>` and its view, which use Boost.LexicalCast.
