# Tuple utilities

Include `<eon/tpl.hpp>` or individual headers in this directory. Names are in
`eon::tpl`; the project targets C++23.

## Iteration and traits

`for_each(tuple, fn)` invokes `fn` on elements in index order, preserving their
reference categories. It accesses elements through `std::get`.
`for_each_tuple_index<Tuple>(fn)` passes
`std::integral_constant<std::size_t, I>` for every index.
`for_each_template_type<Template<T...>>(fn)` passes `std::type_identity<T>`
for each type parameter.

Traits include `make_tuple_index_sequence<Tuple>`, `tuple_all_of`,
`index_sequence_all_of` and invocation/noexcept checks. Tuple size metadata
normalizes cv/ref qualifiers. `aggregate_size_v<T>` uses aggregate-initialization
probes to count top-level aggregate elements; it is not general reflection.

## Stream I/O

`io(tuple)` wraps an existing tuple for stream insertion/extraction.
Output is parenthesized, with `, ` between elements. Strings use double quotes,
characters use single quotes, and nested tuples use the same notation.

```cpp
#include <sstream>
#include <string>
#include <tuple>
#include <eon/tpl.hpp>

std::tuple<int, std::string, char> value{42, "hello", 'x'};
std::stringstream stream;
stream << eon::tpl::io(value); // (42, "hello", 'x')

std::tuple<int, std::string, char> restored;
stream >> eon::tpl::io(restored);
```

Use `io(tuple, separator)` for a custom separator, including an empty one.
The wrapper references the tuple; string separators may also be stored as
views, so keep both alive through the stream operation. Extraction requires
matching parentheses and separators and reports failures through stream state.
Previously read elements may already have changed when later input fails.
The algorithms rely on `std::get` and are intended for standard tuples, pairs
and arrays; the public `eon::tuple_like` concept alone does not guarantee support
for every ADL-only tuple customization.
