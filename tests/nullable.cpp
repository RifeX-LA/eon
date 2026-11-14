#include "test.hpp"
#include <eon/nullable.hpp>
static_assert(eon::nullable<int *> && eon::nullable<std::optional<int>> && eon::nullable<std::unique_ptr<int>>);
static_assert(!eon::nullable<int> && !eon::nullable<void *>);
static_assert(std::same_as<eon::nullable_reference_t<const std::optional<int> &>, const int &>);
static_assert(std::same_as<eon::nullable_reference_t<std::optional<int> &&>, int &&>);
static_assert(std::same_as<eon::nullable_value_t<const int *>, const int>);

struct Node {
    int value = 7;
    std::optional<int> child = 9;
};

TEST(Nullable, InvokesOnlyForPresentInput) {
    Node n;
    Node * p = &n;
    int calls = 0;
    auto fn = [&](Node & v, int x) {
        ++calls;
        return v.value + x;
    };
    EXPECT_EQ(eon::invoke_nullable(fn, p, 2), 9);
    EXPECT_EQ(calls, 1);
    p = nullptr;
    EXPECT_FALSE(eon::invoke_nullable(fn, p, 2));
    EXPECT_EQ(calls, 1);
}

TEST(Nullable, ValueResultIsCopied) {
    Node n;
    Node * p = nullptr;
    EXPECT_EQ(eon::invoke_nullable(&Node::value, &n), 7);
    auto copy = eon::invoke_nullable(&Node::value, &n);
    ASSERT_TRUE(copy.has_value());
    *copy = 99;
    EXPECT_EQ(n.value, 7);
}

TEST(Nullable, ReferenceResultAliasesOriginal) {
    Node n;
    Node * p = nullptr;
    int * ref = eon::invoke_nullable_ref(&Node::value, &n);
    ASSERT_EQ(ref, &n.value);
    *ref = 8;
    EXPECT_EQ(n.value, 8);
    EXPECT_EQ(eon::invoke_nullable_ref(&Node::value, p), nullptr);
}

TEST(Nullable, NestedNullableIsFlattened) {
    Node n;
    Node * p = nullptr;
    EXPECT_EQ(eon::invoke_nullable(&Node::child, &n), 9);
    EXPECT_EQ(eon::invoke_nullable_ref(&Node::child, &n), &*n.child);
    n.child.reset();
    EXPECT_FALSE(eon::invoke_nullable(&Node::child, &n));
    EXPECT_EQ(eon::invoke_nullable_ref(&Node::child, &n), nullptr);
    EXPECT_EQ(eon::invoke_nullable_ref(&Node::child, p), nullptr);
}

TEST(Nullable, VoidCallbackRunsOnlyForPresentInput) {
    Node n;
    Node * p = nullptr;
    int calls = 0;
    auto nothing = [&](Node &) { ++calls; };
    eon::invoke_nullable(nothing, &n);
    eon::invoke_nullable(nothing, p);
    EXPECT_EQ(calls, 1);
}

TEST(Nullable, ValueReturningRefInvocationWrapsResult) {
    Node n;
    EXPECT_EQ(eon::invoke_nullable_ref([](Node &) { return 42; }, &n), 42);
}

TEST(Nullable, EmptyOptionalSkipsCallback) {
    std::optional<int> empty;
    EXPECT_FALSE(eon::invoke_nullable([](int) { return 1; }, empty));
}

TEST(Nullable, UniquePointerInputIsDereferenced) {
    auto owned = std::make_unique<int>(12);
    EXPECT_EQ(eon::invoke_nullable([](int x) { return x * 2; }, owned), 24);
}

TEST(Nullable, RvalueInputTransfersOwnership) {
    std::optional<std::unique_ptr<int>> movable(std::in_place, std::make_unique<int>(42));
    auto moved = eon::invoke_nullable([](std::unique_ptr<int> && x) { return std::move(x); }, std::move(movable));
    ASSERT_TRUE(moved);
    EXPECT_EQ(*moved, 42);
    EXPECT_FALSE(*movable);
}

TEST(Nullable, CallbackExceptionPropagates) {
    Node n;
    EXPECT_THROW(([&] { eon::invoke_nullable([](Node &) -> int { throw std::runtime_error("test"); }, &n); })(),
                 std::runtime_error);
}

TEST(Nullable, ConstReferenceResultPreservesConstness) {
    const Node cn;
    static_assert(std::same_as<decltype(eon::invoke_nullable_ref(&Node::value, &cn)), const int *>);
    EXPECT_EQ(eon::invoke_nullable_ref(&Node::value, &cn), &cn.value);
}
