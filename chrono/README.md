# Overview

This is header-only library that provide `std::chrono` based timers. No external dependencies. All timers are in `eon::chrono` namespace.

### Basic timer

Represented by the `eon::chrono::timer` class.

Features:

1. Initialization

    Use default `rep_type` (`double`) and `period_type` (`std::ratio<1>`, which means seconds)
    ```c++
    eon::chrono::timer timer;
    ```

    Specify `rep_type` and `period_type`
    ```c++
    eon::chrono::timer<int, std::milli> timer;
    ```
   
   **`period_type` must be an instance of `std::ratio`**


2. Get elapsed time of code fragment
   ```c++
    eon::chrono::timer timer;
    // Some work here
    auto elapsed = timer.elapsed();
    ```
   
3. Get elapsed time of any callable object
   ```c++
   auto elapsed = eon::chrono::timer<>::elapsed(
       // any callable object,
       // callable object arguments...
   );
    ```

4. Get elapsed time and return the result of any callable object. The type of the result must not be `void`
   ```c++
   // unpack std::pair
   auto [elapsed, result] = eon::chrono::timer<>::duration_r(
       // any callable object,
       // callable object arguments...
   );
    ```
   
5. Get average execution time
   ```c++
   auto avg = eon::chrono::timer<>::avg_elapsed(
       // count of iterations,
       // any callable object,
       // callable object arguments...        
   );
   ```

6. Reset the timer
    ```c++
    eon::chrono::timer timer;
    // Some work here
    timer.reset();
    ```
   
7. Get the moment when the timer was started (or was reset)
   ```c++
   timer.started_in();
   ```
   
**All the _elapsed_ functions have their analogue for duration**. For example `eon::chrono::timer<>::duration()`, `eon::chrono::timer<>::avg_duration()`, etc.

### Auto timers

These types of timers are automatically output elapsed time in their destructors **with time units**. They have all the features of `eon::chrono::timer`.

Presented by 3 classes:

* `eon::timer::basic_auto_timer` - base class for all auto timers
* `eon::timer::auto_timer` - auto timer for a stream of `char`
* `eon::timer::wauto_timer` - auto timer for a stream of `wchar_t`

Features:

1. Initialization

   ```c++
   std::ofstream ouput("output");   

   eon::chrono::auto_timer auto_timer1; // use default - std::cout and empty prefix message
   eon::chrono::auto_timer auto_timer2(output); // write elapsed time to output with empty prefix message
   eon::chrono::auto_timer auto_timer3(std::cout, "Elapsed time is ") // write elapsed time to output with given prefix message; 
   ```
   
2. Writing time units:
   
   **Time units will be written by default**. If you don't want that, provide `eon::chrono::auto_timer_opt::dont_write_time_unit` value as template parameter.

   ```c++
   eon::chrono::auto_timer<std::uintmax_t, std::milli> auto_timer(std::cout, "Elapsed time is ");
   // some code here...
   ```
   
   Possible output:
   ```
   Elapsed time is 120ms
   ```
   
   ```c++
   eon::chrono::auto_timer<std::uintmax_t, std::milli, eon::chrono::auto_timer_opt::dont_write_time_unit> auto_timer(std::cout, "Elapsed time is ");
   // some code here...
   // time unit will be not written
   ```

# Requirements

C++20