#include "test.hpp"
#include <eon/lifetime.hpp>
std::size_t lifetime_total_from_peer();
std::size_t lifetime_alive_from_peer();

TEST(Lifetime, CountersAreSharedAcrossTranslationUnits) {
    auto alive = eon::lifetime::alive();
    auto total = eon::lifetime::total();
    {
        eon::lifetime value(42);
        EXPECT_EQ(lifetime_alive_from_peer(), alive + 1);
        EXPECT_EQ(lifetime_total_from_peer(), total + 1);
    }
    EXPECT_EQ(lifetime_alive_from_peer(), alive);
}

static_assert(std::is_nothrow_move_constructible_v<eon::lifetime>);
static_assert(std::is_nothrow_move_assignable_v<eon::lifetime>);
static_assert(std::is_nothrow_destructible_v<eon::lifetime>);

TEST(Lifetime, ConstructionAndDestructionUpdateCounters) {
    using L = eon::lifetime;
    auto alive = L::alive(), total = L::total();
    {
        L a;
        L b(std::numeric_limits<int>::min());
        L c(std::numeric_limits<int>::max());
        EXPECT_EQ(a.id(), 0);
        EXPECT_EQ(b.id(), std::numeric_limits<int>::min());
        EXPECT_EQ(c.id(), std::numeric_limits<int>::max());
        EXPECT_EQ(L::alive(), alive + 3);
        EXPECT_EQ(L::total(), total + 3);
        L copy(b);
        L moved(std::move(c));
        EXPECT_EQ(copy.id(), b.id());
        EXPECT_EQ(moved.id(), std::numeric_limits<int>::max());
        EXPECT_EQ(L::alive(), alive + 5);
        EXPECT_EQ(L::total(), total + 5);
    }
    EXPECT_EQ(L::alive(), alive);
    EXPECT_EQ(L::total(), total + 5);
}

TEST(Lifetime, CopyAssignmentCopiesId) {
    eon::lifetime a(1), b(2);
    a = b;
    EXPECT_EQ(a.id(), b.id());
}

TEST(Lifetime, MoveAssignmentCopiesId) {
    eon::lifetime a(1), b(2);
    a = std::move(b);
    EXPECT_EQ(a.id(), 2);
}

TEST(Lifetime, ComparisonsUseId) {
    eon::lifetime low(std::numeric_limits<int>::min()), high(std::numeric_limits<int>::max()), same(low);
    EXPECT_LT(low, high);
    EXPECT_LE(low, same);
    EXPECT_GT(high, low);
    EXPECT_GE(high, low);
    EXPECT_EQ(low, same);
    EXPECT_NE(high, low);
    EXPECT_EQ(low <=> high, std::strong_ordering::less);
}

TEST(Lifetime, StreamFormattingIncludesId) {
    eon::lifetime value(std::numeric_limits<int>::min());
    std::ostringstream os;
    os << value;
    EXPECT_EQ(os.str(), "lifetime(-2147483648)");
}

TEST(Lifetime, StandardFormattingIncludesId) {
    eon::lifetime value(std::numeric_limits<int>::min());
    EXPECT_EQ(std::format("{}", value), "lifetime(-2147483648)");
}
