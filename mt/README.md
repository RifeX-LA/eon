# Overview

This library provides useful functions to work in multithreaded applications.

Features:

1. 4 functions to get number of hardware thread contexts:
    ```c++
    eon::mt::available();
    eon::mt::concurrent_availalbe();
    eon::mt::optimal(size);
    eon::mt::concurrent_optimal(size);
    ```


2. 2 functions (6 overloads) to parallelize _for_ loop:
   ```c++
    eon::mt::for_each(rng, fn);
    eon::mt::for_each(beg, end, fn);
    eon::mt::for_each(exec_policy, rng, fn);
    eon::mt::for_each(exec_policy, beg, end, fn);
    eon::mt::for_each_chunk(rng, fn);
    eon::mt::for_each_chunk(beg, end, fn);
    ```

# Requirements

C++20