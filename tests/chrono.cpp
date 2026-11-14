#include "test.hpp"
#include <eon/chrono.hpp>
namespace c = eon::chrono;
#define UNIT(period, text)                                                                                             \
    static_assert(c::time_unit<period>() == text);                                                                     \
    static_assert(c::wtime_unit<period>() == L##text);                                                                 \
    static_assert(c::u8time_unit<period>() == u8##text);                                                               \
    static_assert(c::u16time_unit<period>() == u##text);                                                               \
    static_assert(c::u32time_unit<period>() == U##text);
UNIT(std::atto, "as");
UNIT(std::femto, "fs");
UNIT(std::pico, "ps");
UNIT(std::nano, "ns");
UNIT(std::micro, "us");
UNIT(std::milli, "ms");
UNIT(std::centi, "cs");
UNIT(std::deci, "ds");
UNIT(std::ratio<1>, "s");
UNIT(std::deca, "das");
UNIT(std::hecto, "hs");
UNIT(std::kilo, "ks");
UNIT(std::mega, "Ms");
UNIT(std::giga, "Gs");
UNIT(std::tera, "Ts");
UNIT(std::peta, "Ps");
UNIT(std::exa, "Es");
using Unknown = std::ratio<7, 11>;
UNIT(Unknown, "");
static_assert(c::get_time_unit<std::milli, char>() == "ms");
static_assert(c::get_time_unit<std::milli, wchar_t>() == L"ms");
static_assert(c::get_time_unit<std::milli, char8_t>() == u8"ms");
static_assert(c::get_time_unit<std::milli, char16_t>() == u"ms");
static_assert(c::get_time_unit<std::milli, char32_t>() == U"ms");
static_assert(std::same_as<c::timer<int, std::milli>::duration_type, std::chrono::duration<int, std::milli>>);

TEST(Chrono, MonotonicTimerAndReset) {
    using T = c::timer<>;
    T t;
    auto before = t.time_point();
    EXPECT_LE(before, T::clock_type::now());
    auto d = t.duration();
    EXPECT_GE(d.count(), 0);
    EXPECT_GE(t.duration(), d);
    EXPECT_GE(t.elapsed(), 0);
    t.reset();
    EXPECT_GE(t.time_point(), before);
}

TEST(Chrono, MeasuresCallbacksAndPreservesResults) {
    using T = c::timer<>;
    int calls = 0;
    EXPECT_GE(T::duration([&] { ++calls; }).count(), 0);
    EXPECT_EQ(calls, 1);
    EXPECT_GE(T::elapsed([&](int n) { calls += n; }, 2), 0);
    EXPECT_EQ(calls, 3);
    auto [dur, r] = T::duration_r([] { return std::make_unique<int>(7); });
    EXPECT_GE(dur.count(), 0);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(*r, 7);
    int value = 5;
    auto pair = T::elapsed_r([&]() -> int & { return value; });
    pair.second = 8;
    EXPECT_EQ(value, 8);
    EXPECT_GE(T::avg_duration(1, [&] { ++calls; }).count(), 0);
    EXPECT_EQ(calls, 4);
    EXPECT_GE(T::avg_elapsed(3, [&] { ++calls; }), 0);
    EXPECT_EQ(calls, 7);
}

TEST(Chrono, ZeroIterationsRejected) {
    using T = c::timer<>;
    int calls = 7;
    // Reject zero before invoking the callback.
    EXPECT_THROW(T::avg_elapsed(0, [&] { ++calls; }), std::invalid_argument);
    EXPECT_EQ(calls, 7);
    EXPECT_THROW([] { T::duration([] { throw std::runtime_error("test"); }); }(), std::runtime_error);
}

TEST(Chrono, AutoTimerOutput) {
    std::ostringstream os;
    {
        c::auto_timer<int, std::milli> timer(os);
        EXPECT_EQ(&timer.stream(), &os);
        EXPECT_TRUE(os.str().empty());
    }
    EXPECT_TRUE(os.str().ends_with("ms"));
    std::ostringstream plain;
    {
        c::auto_timer<int, std::ratio<1>, c::writing_time_unit::disable> timer(plain);
    }
    EXPECT_FALSE(plain.str().empty());
    EXPECT_EQ(plain.str().find('s'), std::string::npos);
    std::wostringstream wide;
    {
        c::wauto_timer<int, std::milli> timer(wide);
    }
    EXPECT_TRUE(wide.str().ends_with(L"ms"));
}

// All five character encodings are also checked by UNIT below.
TEST(Chrono, ExasecondUsesSecondsSuffix) {
    EXPECT_EQ(c::time_unit<std::exa>(), "Es");
}

template <typename T>
class TimerCountRegression : public ::testing::Test {};

using TimerReps = ::testing::Types<int, long long, float, double>;
TYPED_TEST_SUITE(TimerCountRegression, TimerReps);

TYPED_TEST(TimerCountRegression, RejectsZeroBeforeCallingCallback) {
    using Timer = eon::chrono::timer<TypeParam>;
    int calls = 0;
    auto callback = [&] { ++calls; };
    EXPECT_THROW(Timer::avg_duration(0, callback), std::invalid_argument);
    EXPECT_THROW(Timer::avg_elapsed(0, callback), std::invalid_argument);
    EXPECT_EQ(calls, 0);
    EXPECT_GE(Timer::avg_duration(1, callback).count(), 0);
    EXPECT_EQ(calls, 1);
    EXPECT_GE(Timer::avg_elapsed(3, callback), 0);
    EXPECT_EQ(calls, 4);
}
