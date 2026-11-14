# Multithreading

Include `<eon/mt.hpp>`. Public names are in `eon::mt`. The project targets
C++23, including `std::jthread` and container range operations.

## Concurrency and algorithms

`available()` returns at least 1; `concurrent_available()` returns at least 2.
They use `std::thread::hardware_concurrency()` when its value is larger.
`optimal(n)` and `concurrent_optimal(n)` return the minimum of `n` and the
corresponding available count, so they return zero for zero work.

Algorithms accept a forward range or an iterator/sentinel pair:

- `for_each(range, fn)` splits work between joined worker threads and the caller.
- `for_each(policy, range, fn)` delegates to the standard execution-policy algorithm.
- `for_each_chunk(range, fn)` invokes callbacks with iterator pairs or subranges.
- `for_each_async(range, fn, policy)` returns one future per element.
- `for_each_chunk_async(range, fn, policy)` returns futures for chunks.

Async overloads default to `std::launch::async | std::launch::deferred`.
Keep referenced ranges and callback state alive until their tasks finish.
Callbacks may run concurrently and must synchronize shared state. Exceptions
from async tasks are retrieved through futures; exceptions escaping a worker
thread in the synchronous algorithms terminate the program.

## Thread pool

`thread_pool<R = void>` executes nullary tasks with results convertible to `R`.
`add_task(fn)` returns `std::future<R>`, including exceptions from the task.

```cpp
#include <eon/mt.hpp>

eon::mt::thread_pool<int> pool(2);
auto result = pool.add_task([] { return 42; });
int value = result.get();
pool.wait();
```

`resize` changes worker count. `force_resize` requests selected workers to exit
between tasks; it does not interrupt an executing callable. `request_stop`
requests stopping; `wait` also joins workers. Workers drain queued tasks before
a normal stop. Do not submit more work after stopping. A zero-worker pool cannot
execute queued work until workers are added. Coordinate task submission and
pool lifecycle operations externally.

## Multiple-mutex lock

`unique_lock<Mutexes...>` owns one or more distinct mutexes. It supports normal,
deferred, adopted and try-lock construction, move, `lock`, `try_lock`,
`unlock`, `owns_lock` and `swap`. Lock tags precede mutex arguments:

```cpp
#include <mutex>
#include <eon/mt.hpp>

std::mutex first, second;
eon::mt::unique_lock lock(std::defer_lock, first, second);
lock.lock();
```

Mutexes must outlive the lock. Linux tests link TBB when available because
libstdc++ uses it for standard parallel algorithms.
