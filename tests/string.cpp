#include "test.hpp"
#include <eon/spark/compare.hpp>
#include <eon/spark/string.hpp>
#include <eon/spark/views.hpp>
#include <list>
#include <span>
namespace s = eon::spark;

TEST(String, EmptyNullBackedRangesAndHighBytesAreRejected) {
    EXPECT_FALSE(s::from_str<int>(std::string_view{}));
    EXPECT_FALSE(s::from_str<int>(std::span<const char>{}));
    EXPECT_FALSE(s::from_str<int>(nullptr, nullptr));
    const std::string high_byte(1, static_cast<char>(0xff));
    EXPECT_FALSE(s::from_str<int>(high_byte));
}

TEST(String, MultipleSignsAreRejected) {
    for (auto text : {"+-1", "++1", "--1", "-+1"}) {
        EXPECT_FALSE(s::from_str<int>(std::string_view(text)));
        EXPECT_FALSE(s::from_str<double>(std::string_view(text)));
    }
}

using namespace std::literals;

template <typename T>
class IntegerRoundTrip : public ::testing::Test {};

using IntegerTypes = ::testing::Types<signed char, unsigned char, short, unsigned short, int, unsigned, long,
                                      unsigned long, long long, unsigned long long>;
TYPED_TEST_SUITE(IntegerRoundTrip, IntegerTypes);

TYPED_TEST(IntegerRoundTrip, EveryBaseAndBoundary) {
    using T = TypeParam;
    for (T value : {std::numeric_limits<T>::lowest(), T{0}, T{1}, std::numeric_limits<T>::max()}) {
        for (int base = 2; base <= 36; ++base) {
            SCOPED_TRACE(::testing::Message() << "value=" << +value << ", base=" << base);
            auto text = s::to_str(value, base);
            auto result = s::from_str<T, s::str_parsing::strict>(text, base);
            ASSERT_TRUE(result.has_value());
            EXPECT_EQ(*result, value);
            std::string out;
            s::to_str(value, std::back_inserter(out), base);
            EXPECT_EQ(out, text);
        }
    }
}

TEST(String, InvalidInputAndIntegerOverflow) {
    for (auto text : {""sv, " "sv, "+"sv, "-"sv, "abc"sv, "++1"sv}) {
        auto r = s::from_str<int>(text);
        ASSERT_FALSE(r);
        EXPECT_EQ(r.error(), std::errc::invalid_argument);
    }
    for (auto text : {"2147483648"sv, "-2147483649"sv}) {
        auto r = s::from_str<std::int32_t>(text);
        ASSERT_FALSE(r);
        EXPECT_EQ(r.error(), std::errc::result_out_of_range);
    }
    auto overflow = s::from_str<unsigned long long>("18446744073709551616"sv);
    ASSERT_FALSE(overflow);
    EXPECT_EQ(overflow.error(), std::errc::result_out_of_range);
    EXPECT_FALSE(s::from_str<unsigned>("-1"sv));
    EXPECT_FALSE(s::from_str<unsigned char>("256"sv));
}

TEST(String, ParsingModesAndRangeOverloads) {
    EXPECT_EQ(s::from_str<int>(" \t\r\n+42tail"sv), 42);
    EXPECT_EQ(s::from_str<int>("42 "sv), 42);
    EXPECT_TRUE((!s::from_str<int, s::str_parsing::strict>("42 "sv)));
    EXPECT_EQ((s::from_str<int, s::str_parsing::strict>(" +42"sv)), 42);
    EXPECT_EQ(s::from_str<int>("ff"sv, 16), 255);
    EXPECT_EQ(s::from_str<int>("101"sv, 2), 5);
    std::list<char> list{'1', '2', '3'};
    EXPECT_EQ(s::from_str<int>(list), 123);
    const char chars[]{'x', '4', '2', 'y'};
    EXPECT_EQ(s::from_str<int>(chars + 1, chars + 3), 42);
    EXPECT_FALSE(s::from_str<int>(chars, chars));
    const std::string embedded("12\0x", 4);
    EXPECT_EQ(s::from_str<int>(embedded), 12);
    EXPECT_TRUE((!s::from_str<int, s::str_parsing::strict>(embedded)));
}

TEST(String, FloatingPointRoundTripAndRangeErrors) {
    for (double value : {0., -0., 1.5, -2.25, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(),
                         std::numeric_limits<double>::denorm_min()}) {
        auto text = s::to_str(value);
        auto parsed = s::from_str<double, s::str_parsing::strict>(text);
        ASSERT_TRUE(parsed.has_value()) << "input=" << text;
        EXPECT_EQ(*parsed, value);
        if (value == 0.) {
            EXPECT_EQ(std::signbit(*parsed), std::signbit(value));
        }
    }
    for (auto text : {"1e9999"sv, "1e-9999"sv}) {
        auto r = s::from_str<double>(text);
        ASSERT_FALSE(r);
        EXPECT_EQ(r.error(), std::errc::result_out_of_range);
    }
    EXPECT_EQ(s::from_str<double>("1.25tail"sv), 1.25);
    EXPECT_TRUE((!s::from_str<double, s::str_parsing::strict>("1.25tail"sv)));
    auto infinity = s::from_str<double>("inf"sv);
    auto nan = s::from_str<double>("nan"sv);
    ASSERT_TRUE(infinity.has_value());
    ASSERT_TRUE(nan.has_value());
    EXPECT_TRUE(std::isinf(*infinity));
    EXPECT_TRUE(std::isnan(*nan));
}

TEST(String, StoredConversionResultsAndOutputAdaptors) {
    std::array inputs{"1"sv, "bad"sv, "3"sv};
    // indirect requires stored wrapper objects; materialize conversion results
    // first.
    std::vector<std::expected<int, std::errc>> parsed;
    for (auto item : inputs | s::views::from_str<int>) {
        parsed.push_back(item);
    }
    EXPECT_TRUE(std::ranges::equal(parsed | s::views::has_value | s::views::indirect, std::array{1, 3}));
    EXPECT_TRUE(std::ranges::equal(std::array{0, -1, 42} | s::views::to_str, std::array{"0"s, "-1"s, "42"s}));
    EXPECT_EQ(s::from_str<double>("1.5e2"sv, std::chars_format::scientific), 150.);
    EXPECT_EQ(s::from_str<double>("1.8"sv, std::chars_format::hex), 1.5);
    std::string scientific;
    s::to_str(1.5, std::back_inserter(scientific), std::chars_format::scientific);
    EXPECT_EQ((s::from_str<double, s::str_parsing::strict>(scientific, std::chars_format::scientific)), 1.5);
    char output[32]{};
    auto end = s::to_str(123, output);
    EXPECT_EQ(end, output + 3);
    EXPECT_EQ(std::string_view(output, 3), "123");
}

static_assert(std::ranges::equal(s::make_input_str("abc"), std::string_view("abc")));
static_assert(std::ranges::empty(s::make_input_str("")));

template <typename T>
class InputStringRegression : public ::testing::Test {};

using Characters = ::testing::Types<char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>;
TYPED_TEST_SUITE(InputStringRegression, Characters);

TYPED_TEST(InputStringRegression, StopsAtFirstZeroAndComposes) {
    using C = TypeParam;
    const C text[]{C('a'), C('b'), C{}, C('c'), C{}};
    EXPECT_TRUE(std::ranges::equal(s::make_input_str(text), std::array{C('a'), C('b')}));
    const C empty[]{C{}};
    EXPECT_TRUE(std::ranges::empty(s::make_input_str(empty)));
    std::array<const C *, 2> pointers{text, empty};
    auto views = pointers | s::views::make_input_str;
    EXPECT_EQ(std::ranges::distance(*views.begin()), 2);
    EXPECT_TRUE(std::ranges::empty(*++views.begin()));
}
