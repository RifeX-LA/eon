# Lifetime

Include `<eon/lifetime.hpp>`. `eon::lifetime` logs construction, assignment,
comparison and destruction through `std::println`. It requires C++23 printing
and formatting support.

```cpp
#include <eon/lifetime.hpp>

int main() {
    eon::lifetime first(7);
    auto second = first;
    auto id = second.id();               // 7
    auto living = eon::lifetime::alive(); // 2 at this point
}
```

`total()` counts constructions since program start, including copies and moves.
`alive()` counts currently living instances. Assignment changes the ID but does
not increment construction counters. Counters are shared across translation
units and are not synchronized for concurrent access.

Comparisons use IDs. Stream insertion and `std::format` produce
`lifetime(id)`; these operations and comparisons take operands by value and
may themselves log copies and destruction. Move construction copies the ID
without clearing the source.
