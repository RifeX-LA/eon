# Tests

The suite uses GoogleTest, CMake (3.26 or newer) and CTest. It requires a C++23
compiler and standard library supporting `std::expected`, `std::print`,
`std::bind_back`, range adaptor closures and container range operations.
Keep the checkout directory named `eon` for the `<eon/...>` includes.

## Dependencies and local execution

Boost headers are required for the Asio tests; no compiled Boost libraries are
needed. Point `EON_BOOST_INCLUDE_DIR` at the directory containing `boost/asio.hpp`.
CMake looks for GoogleTest >= 1.17 first, then downloads a pinned 1.17.0 archive.
For offline configuration, install GoogleTest or pass
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest`.

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON -DEON_BOOST_INCLUDE_DIR=/path/to/boost/include
cmake --build build-tests --config Debug --target check --parallel
```

The `check` target builds all test executables and independent header checks,
then runs CTest. For a single-configuration generator such as Ninja, set
`-DCMAKE_BUILD_TYPE=Debug` (or Release) during configuration.

```sh
ctest --test-dir build-tests -C Debug --output-on-failure --no-tests=error
ctest --test-dir build-tests -C Debug -N
ctest --test-dir build-tests -C Debug -R IntegerRoundTrip
```

Eleven executables cover concepts, utility, spark, string conversion, nullable,
scope guards, tuple utilities, chrono, multithreading, lifetime and Asio.
An object-library target compiles each module header in its own translation
unit. Lifetime also has a second test translation unit. Tests include
`static_assert` checks and runtime GoogleTest assertions. CTest registers
individual scenarios with a 30-second timeout. The suite does not measure a
line or branch coverage percentage.

Asio tests use loopback TCP sockets, ephemeral ports and isolated temporary
files. Thread tests make assertions about invariants instead of a fixed CPU
count. Test sources live alongside this README; the build definition is
[CMakeLists.txt](CMakeLists.txt).

## CLion

Open the repository's root `CMakeLists.txt`, reload CMake and select the shared
[All tests](../.run/All%20tests.run.xml) configuration. It builds all targets
before running CTest. Its default profile is Debug; select another configured
profile to use Release.

The configuration uses the CTest bundled with CLion for Windows, located
relative to `$APPLICATION_HOME_DIR$`. On other platforms select the toolchain's
CTest executable in the configuration, or build the `check` target.

## AddressSanitizer

Use a separate build directory. This option also builds GoogleTest from source
with matching instrumentation. On Windows run from a Visual Studio Developer
terminal so the ASan runtime is available.

```sh
cmake -S . -B build-tests/asan -DEON_TEST_ASAN=ON -DEON_BOOST_INCLUDE_DIR=/path/to/boost/include
cmake --build build-tests/asan --config Debug --parallel
ctest --test-dir build-tests/asan -C Debug --output-on-failure --no-tests=error
```

## GitHub Actions and formatting

[CI](../.github/workflows/ci.yml) runs on pushes, pull requests and manual
dispatch. For each compiler it configures Debug, Release and Debug with ASan,
builds all test targets (including header checks and `static_assert`s), and
runs CTest:

| Compiler | Runner | Standard library |
| --- | --- | --- |
| MSVC / Visual Studio 2026 | Windows 2025 VS 2026 | MSVC STL |
| GCC 15 | Ubuntu 26.04 | libstdc++ 15 |
| Clang 22 | Ubuntu 26.04 | libstdc++ 15, explicitly selected |

Linux jobs install TBB for the libstdc++ parallel algorithm backend; CMake links
it when its package is available. Every compiler job uses Boost 1.92.0 with
archive SHA-256 verification and uploads CTest JUnit reports/logs when available.
A push with several commits checks its final revision.

A separate `clang-format` job checks all tracked C++ sources with version 22.1.3
and fails on formatting differences. The [root configuration](../.clang-format)
and [test configuration](.clang-format) set a 120-column limit and the
project's template, pointer/reference and function formatting rules.
