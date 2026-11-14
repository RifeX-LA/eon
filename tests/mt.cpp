#include "test.hpp"
#include <atomic>
#include <eon/mt.hpp>
#include <numeric>

struct FakeMutex {
    bool locked = false, fail = false;

    void lock() {
        if (locked) {
            throw std::logic_error("locked");
        }
        locked = true;
    }

    bool try_lock() {
        if (fail || locked) {
            return false;
        }
        locked = true;
        return true;
    }

    void unlock() {
        locked = false;
    }
};

struct ChunkSum {
    template <typename It>
    int operator()(It b, It e) const {
        return std::accumulate(b, e, 0);
    }

    template <typename R>
    int operator()(R r) const {
        return (*this)(r.begin(), r.end());
    }
};
namespace m = eon::mt;

TEST(Mt, ConcurrencyLimits) {
    EXPECT_GE(m::available(), 1);
    EXPECT_GE(m::concurrent_available(), 2);
    EXPECT_EQ(m::optimal(0), 0);
    EXPECT_EQ(m::optimal(1), 1);
    EXPECT_EQ(m::optimal(std::numeric_limits<unsigned>::max()), m::available());
    EXPECT_EQ(m::concurrent_optimal(0), 0);
    EXPECT_EQ(m::concurrent_optimal(1), 1);
    EXPECT_EQ(m::concurrent_optimal(std::numeric_limits<unsigned>::max()), m::concurrent_available());
}

TEST(Mt, BalancedChunkPartitioning) {
    for (std::size_t size : {0u, 1u, 2u, 7u, 32u, 101u}) {
        for (std::size_t threads : {1u, 2u, 3u, 16u}) {
            auto chunks = m::detail::get_chunks_sizes(size, threads);
            EXPECT_EQ(chunks.size(), threads);
            EXPECT_EQ(std::accumulate(chunks.begin(), chunks.end(), std::size_t{}), size);
            EXPECT_LE(*std::ranges::max_element(chunks) - *std::ranges::min_element(chunks), 1);
        }
    }
}

TEST(Mt, LockOwnershipMovesAndErrors) {
    FakeMutex a, b;
    {
        m::unique_lock guard(a);
        EXPECT_TRUE(guard.owns_lock());
        EXPECT_TRUE(a.locked);
        EXPECT_THROW(([&] { guard.lock(); })(), std::system_error);
        EXPECT_THROW(([&] { (void)guard.try_lock(); })(), std::system_error);
        guard.unlock();
        EXPECT_FALSE(guard);
        EXPECT_FALSE(a.locked);
        EXPECT_THROW(([&] { guard.unlock(); })(), std::system_error);
        EXPECT_TRUE(guard.try_lock());
    }
    EXPECT_FALSE(a.locked);
    {
        m::unique_lock guard(std::defer_lock, a, b);
        EXPECT_FALSE(guard);
        guard.lock();
        EXPECT_TRUE(a.locked && b.locked);
        m::unique_lock moved(std::move(guard));
        EXPECT_TRUE(moved);
        EXPECT_FALSE(guard);
    }
    EXPECT_FALSE(a.locked);
    EXPECT_FALSE(b.locked);
    b.fail = true;
    {
        m::unique_lock guard(std::try_to_lock, a, b);
        EXPECT_FALSE(guard);
        EXPECT_FALSE(a.locked);
    }
    b.fail = false;
    a.lock();
    {
        m::unique_lock guard(std::adopt_lock, a);
        EXPECT_TRUE(guard);
    }
    EXPECT_FALSE(a.locked);
    {
        m::unique_lock first(a);
        m::unique_lock second(b);
        first = std::move(second);
        EXPECT_FALSE(a.locked);
        EXPECT_TRUE(b.locked);
        EXPECT_TRUE(first && !second);
    }
}

TEST(Mt, RangeForEachVisitsEveryElement) {
    for (int size : {0, 1, 2, 17, 257}) {
        SCOPED_TRACE(size);
        std::vector<int> v(size, 0);
        m::for_each(v, [](int & n) { ++n; });
        EXPECT_TRUE(std::ranges::all_of(v, [](int n) { return n == 1; }));
    }
}

TEST(Mt, IteratorForEachVisitsEveryElement) {
    for (int size : {0, 1, 2, 17, 257}) {
        SCOPED_TRACE(size);
        std::vector<int> v(size, 0);
        m::for_each(v.begin(), v.end(), [](int & n) { ++n; });
        EXPECT_TRUE(std::ranges::all_of(v, [](int n) { return n == 1; }));
    }
}

TEST(Mt, ExecutionPolicyForEachVisitsEveryElement) {
    for (int size : {0, 1, 2, 17, 257}) {
        SCOPED_TRACE(size);
        std::vector<int> v(size, 0);
        m::for_each(std::execution::seq, v, [](int & n) { ++n; });
        EXPECT_TRUE(std::ranges::all_of(v, [](int n) { return n == 1; }));
    }
}

TEST(Mt, DeferredForEachReturnsOneFuturePerElement) {
    for (int size : {0, 1, 2, 17, 257}) {
        SCOPED_TRACE(size);
        std::vector<int> v(size, 0);
        auto futures = m::for_each_async(v, [](int & n) { return ++n; }, std::launch::deferred);
        EXPECT_EQ(futures.size(), v.size());
        for (auto & f : futures) {
            EXPECT_EQ(f.get(), 1);
        }
    }
}

TEST(Mt, ChunkAsyncCoversEntireRange) {
    for (int size : {0, 1, 2, 17, 257}) {
        SCOPED_TRACE(size);
        std::vector<int> v(size, 0);
        std::ranges::fill(v, 4);
        auto chunks = m::for_each_chunk_async(v, ChunkSum{}, std::launch::async);
        int sum = 0;
        for (auto & f : chunks) {
            sum += f.get();
        }
        EXPECT_EQ(sum, size * 4);
    }
}

TEST(Mt, AsyncExceptionsAreStoredInFutures) {
    std::vector<int> v{1, 2};
    auto fs = m::for_each_async(v, [](int) -> int { throw std::runtime_error("test"); }, std::launch::async);
    for (auto & f : fs) {
        EXPECT_THROW(([&] { (void)f.get(); })(), std::runtime_error);
    }
}

TEST(Mt, ThreadPoolTasksExceptionsAndResizing) {
    {
        m::thread_pool<int> pool(2);
        EXPECT_EQ(pool.size(), 2);
        std::vector<std::future<int>> results;
        for (int i = 0; i < 100; ++i) {
            results.push_back(pool.add_task([i] { return i * i; }));
        }
        for (int i = 0; i < 100; ++i) {
            EXPECT_EQ(results[i].get(), i * i);
        }
        auto ex = pool.add_task([]() -> int { throw std::runtime_error("test"); });
        EXPECT_THROW(([&] { (void)ex.get(); })(), std::runtime_error);
        pool.resize(3);
        EXPECT_EQ(pool.size(), 3);
        pool.resize(1);
        EXPECT_EQ(pool.size(), 1);
        pool.force_resize(3);
        EXPECT_EQ(pool.size(), 3);
        pool.force_resize(1);
        EXPECT_EQ(pool.size(), 1);
        EXPECT_EQ(pool.add_task([p = std::make_unique<int>(7)] { return *p; }).get(), 7);
        pool.wait();
    }
}

TEST(Mt, ThreadPoolStartsWithZeroWorkers) {
    {
        m::thread_pool<> pool(0);
        EXPECT_EQ(pool.size(), 0);
        auto f = pool.add_task([] {});
        pool.resize(1);
        f.get();
        pool.wait_and_deallocate();
        EXPECT_EQ(pool.size(), 0);
    }
}
