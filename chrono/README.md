# Chrono

Include `<eon/chrono.hpp>`. Timers are in `eon::chrono` and use
`std::chrono::steady_clock`. The project targets C++23.

## Timer

`timer<Rep = double, Period = std::ratio<1>>` defaults to floating-point seconds.
`Period` must be a duration period accepted by `std::chrono::duration`.

```cpp
#include <eon/chrono.hpp>

eon::chrono::timer<> timer;
auto elapsed = timer.elapsed();       // Rep: double seconds
auto duration = timer.duration();     // std::chrono::duration<double>
auto started = timer.time_point();    // last construction/reset time
timer.reset();

auto [work_duration, result] =
    eon::chrono::timer<>::duration_r([] { return 42; });
auto average = eon::chrono::timer<>::avg_elapsed(100, [] {});
```

Static `duration(fn, args...)` and `elapsed(fn, args...)` measure one invocation.
The `_r` versions return a pair of the measurement and the non-void invocation
result, preserving its reference type. `avg_duration` and `avg_elapsed` invoke
the callable `count` times; zero throws `std::invalid_argument` before invoking
it. Integral representations truncate fractional ticks.

## Automatic output

`basic_auto_timer<Rep, Period, CharT, Traits, Opt>` derives from `timer` and
requires a stream in its constructor. Destruction writes the elapsed count and,
by default, its unit suffix. There is no default output stream or prefix argument.

```cpp
#include <iostream>
#include <eon/chrono.hpp>

{
    std::cout << "Elapsed: ";
    eon::chrono::auto_timer<double, std::milli> timer(std::cout);
    // Destruction writes a value such as 1.25ms, without a newline.
}
```

Aliases exist for `auto_timer`, `wauto_timer`, `u8auto_timer`, `u16auto_timer`
and `u32auto_timer`. They select the stream character type; the stream must
support numeric insertion for that type. Pass
`writing_time_unit::disable` as the alias's third template argument to omit
the suffix. The referenced stream must outlive the timer. Destructors are
`noexcept`, so stream insertion must not throw.
