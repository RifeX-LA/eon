# Eon

Eon is a header-only C++ library. The root CMake target requires C++23 and a
standard library implementing the C++23 features used by the selected modules.

| Module | Header | Description |
| --- | --- | --- |
| [asio](asio/README.md) | `<eon/asio.hpp>` | Synchronous TCP serialization and deadlines |
| [chrono](chrono/README.md) | `<eon/chrono.hpp>` | Steady-clock timers and automatic stream output |
| [concepts](concepts/README.md) | `<eon/concepts.hpp>` | Type, callable, tuple and range concepts |
| [lifetime](lifetime/README.md) | `<eon/lifetime.hpp>` | Object lifetime logging and counters |
| [mt](mt/README.md) | `<eon/mt.hpp>` | Parallel algorithms, thread pool and multiple-mutex lock |
| [nullable](nullable/README.md) | `<eon/nullable.hpp>` | Conditional invocation through nullable objects |
| [scope_guard](scope_guard/README.md) | `<eon/scope_guard.hpp>` | Scope-exit callbacks |
| [spark](spark/README.md) | `<eon/spark.hpp>` | Function objects, comparisons, conversions and range adaptors |
| [tpl](tpl/README.md) | `<eon/tpl.hpp>` | Tuple algorithms, traits and stream I/O |
| [utility](utility/README.md) | `<eon/utility.hpp>` | Callable forwarding, wrappers and type-pack iteration |

## Usage

Keep the checkout directory named `eon` and add its parent to your include path.
Alternatively, add this directory with CMake and link the interface target:

```cmake
add_subdirectory(eon)
target_link_libraries(my_app PRIVATE eon)
```

Set `BUILD_TESTING=OFF` when embedding the library if you do not want to build its
tests. Most modules use only the standard library. Asio requires Boost headers;
the optional Spark Boost adapter also requires Boost when enabled.

See [tests/README.md](tests/README.md) for dependencies, local test commands and
the GitHub Actions compiler matrix.
