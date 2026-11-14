#include "test.hpp"
#include <eon/scope_guard.hpp>

struct Noop {
    constexpr void operator()() const noexcept {
    }
};

static_assert(!std::is_copy_constructible_v<eon::scope_guard<Noop>>);
static_assert(!std::is_move_constructible_v<eon::scope_guard<Noop>>);
static_assert(!std::is_copy_assignable_v<eon::cancelable_scope_guard<Noop>>);
static_assert(!std::is_move_constructible_v<eon::cancelable_scope_guard<Noop>>);
static_assert(std::is_nothrow_destructible_v<eon::scope_guard<Noop>>);
static_assert([] {
    int n = 0;
    {
        eon::scope_guard g([&] { ++n; });
    }
    return n == 1;
}());
static_assert([] {
    int n = 0;
    {
        eon::cancelable_scope_guard g([&] { ++n; });
        g.reset();
        g.reset();
    }
    return n == 0;
}());
static_assert([] {
    int n = 0;
    {
        eon::cancelable_scope_guard g([&] { ++n; });
        g.release();
    }
    return n == 1;
}());

TEST(ScopeGuard, ExecutesDuringStackUnwinding) {
    int calls = 0;
    try {
        eon::scope_guard guard([&] { ++calls; });
        throw 1;
    } catch (int) {
    }
    EXPECT_EQ(calls, 1);
}

TEST(ScopeGuard, ExecutesInReverseScopeOrder) {
    std::vector<int> order;
    {
        eon::scope_guard a([&] { order.push_back(1); });
        eon::scope_guard b([&] { order.push_back(2); });
    }
    EXPECT_EQ(order, (std::vector<int>{2, 1}));
}

TEST(ScopeGuard, AcceptsMoveOnlyCallback) {
    int calls = 0;
    {
        eon::scope_guard guard([p = std::make_unique<int>(3), &calls] { calls += *p; });
    }
    EXPECT_EQ(calls, 3);
}

TEST(ScopeGuard, ContainerReleaseExecutesExactlyOnce) {
    int calls = 0;
    {
        eon::scope_guard_container guards;
        EXPECT_TRUE(guards.empty());
        guards.reserve(8);
        guards.push_back([&] { ++calls; });
        guards.push_back([&] { calls += 2; });
        EXPECT_FALSE(guards.empty());
        guards.release();
        EXPECT_TRUE(guards.empty());
        EXPECT_EQ(calls, 3);
    }
    EXPECT_EQ(calls, 3);
}

TEST(ScopeGuard, ContainerResetDiscardsCallbacks) {
    int calls = 0;
    {
        eon::scope_guard_container guards;
        guards.push_back([&] { ++calls; });
        guards.reset();
        guards.reset();
    }
    EXPECT_EQ(calls, 0);
}

TEST(ScopeGuard, ContainerConstructsFromMoveOnlyRange) {
    int calls = 0;
    {
        std::array<std::move_only_function<void()>, 2> functions{[&] { ++calls; }, [&] { calls += 2; }};
        eon::scope_guard_container guards(std::move(functions) | std::views::as_rvalue);
    }
    EXPECT_EQ(calls, 3);
}

TEST(ScopeGuard, ContainerAppendsMoveOnlyRange) {
    int calls = 0;
    {
        eon::scope_guard_container guards;
        std::array<std::move_only_function<void()>, 2> functions{[&] { ++calls; }, [&] { calls += 2; }};
        guards.append_range(std::move(functions) | std::views::as_rvalue);
    }
    EXPECT_EQ(calls, 3);
}

TEST(ScopeGuard, ContainerMoveTransfersCallbacks) {
    int calls = 0;
    {
        eon::scope_guard_container a;
        a.push_back([&] { ++calls; });
        eon::scope_guard_container b(std::move(a));
    }
    EXPECT_EQ(calls, 1);
}
