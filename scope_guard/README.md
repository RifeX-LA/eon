# Scope guards

Include `<eon/scope_guard.hpp>`. Names are in `eon`; the project targets C++23.

```cpp
#include <eon/scope_guard.hpp>

int calls = 0;
{
    eon::scope_guard guard([&] { ++calls; });
} // calls == 1
```

- `scope_guard<Fn>` invokes its stored callable at destruction. It cannot be copied or moved.
- `cancelable_scope_guard<Fn>` adds `reset()` to discard the callback and
  `release()` to invoke it immediately, then discard it. Call `release()`
  only while a callback is still stored. It also cannot be copied or moved.
- `scope_guard_container` stores move-only callbacks. It supports `push_back`,
  `append_range`, `reserve`, `empty`, `reset` and `release`. Stored callbacks
  run in insertion order, both at destruction and during `release()`.
  `reset()` discards callbacks without calling them.

These are unconditional scope-exit guards, including during exception unwinding.
All guard destructors are `noexcept`: callbacks invoked there must not throw.
Any objects referenced by callbacks must remain alive until invocation.
