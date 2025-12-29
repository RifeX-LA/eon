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


2. 4 functions (10 overloads) to parallelize _for_ loop:
   ```c++
    eon::mt::for_each(rng, fn);
    eon::mt::for_each(beg, end, fn);
    eon::mt::for_each(exec_policy, rng, fn);
    eon::mt::for_each(exec_policy, beg, end, fn);
    eon::mt::for_each_chunk(rng, fn);
    eon::mt::for_each_chunk(beg, end, fn);
   
    eon::mt::for_each_async(beg, end, fn, policy);
    eon::mt::for_each_async(rng, fn, policy);
    eon::mt::for_each_chunk_async(rng, fn, policy);
    eon::mt::for_each_chunk_async(beg, end, fn, policy);
    ```
      
   All the `_async` functions return `std::vector<std::future<T>>`, where `T` is the invoke result of `fn`   


3. thread pool (`eon::mt::thread_pool<R>`)

# Requirements

C++20