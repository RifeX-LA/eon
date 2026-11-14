#include "test.hpp"
#include <eon/spark/compare.hpp>
#include <eon/tpl.hpp>
#include <forward_list>

TEST(Tpl, SeparatorMismatchRestoresConsumedInput) {
    std::istringstream input("ax!");
    EXPECT_FALSE(eon::tpl::detail::accept_range(input, std::string_view("ab")));
    EXPECT_TRUE(input.fail());
    input.clear();
    std::string remaining;
    std::getline(input, remaining);
    EXPECT_EQ(remaining, "ax!");
}

TEST(Tpl, QuotedCharacterExtractionUpdatesDestination) {
    char symbol = 'a';
    std::istringstream input("'b'");
    input >> eon::tpl::detail::quoted(symbol);
    EXPECT_FALSE(input.fail());
    EXPECT_EQ(symbol, 'b');
}

struct Empty {};

struct Flat {
    int a;
    char b;
    double c;
};

struct Nested {
    int a[3];
    char b;
};

static_assert(eon::tpl::aggregate_size_v<Empty> == 0);
static_assert(eon::tpl::aggregate_size_v<Flat> == 3);
static_assert(eon::tpl::aggregate_size_v<Nested> == 2);
static_assert(eon::tpl::aggregate_size_v<int[5]> == 5);
static_assert(std::same_as<eon::tpl::make_tuple_index_sequence<std::tuple<>>, std::index_sequence<>>);
static_assert(eon::tpl::tuple_invocable<decltype([](int) {}), std::tuple<int, int>>);
static_assert(!eon::tpl::tuple_invocable<decltype([](int) {}), std::tuple<std::string>>);
static_assert(eon::tpl::is_nothrow_tuple_invocable_v<decltype([](auto) noexcept {}), std::tuple<int, char>>);
static_assert(!eon::tpl::is_nothrow_tuple_invocable_v<decltype([](auto) {}), std::tuple<int>>);
static_assert(eon::tpl::is_nothrow_tuple_invocable_v<decltype([](int) {}), std::tuple<>>);
static_assert([] {
    auto t = std::tuple{1, 2, 3};
    eon::tpl::for_each(t, [](int & x) { x *= 2; });
    return t == std::tuple{2, 4, 6};
}());
static_assert([] {
    int n = 0;
    eon::tpl::for_each(std::tuple{}, [&](auto) { ++n; });
    return n == 0;
}());
static_assert([] {
    int n = 0;
    eon::tpl::for_each_tuple_index<std::tuple<int, int, int>>([&](auto i) { n = n * 10 + int(i) + 1; });
    return n == 123;
}());
static_assert([] {
    int n = 0;
    eon::tpl::for_each_template_type<std::tuple<int, char, double>>([&](auto) { ++n; });
    return n == 3;
}());

TEST(Tpl, MovesTupleElements) {
    auto move = std::tuple{std::make_unique<int>(5)};
    std::unique_ptr<int> result;
    eon::tpl::for_each(std::move(move), [&](auto && p) { result = std::move(p); });
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 5);
    EXPECT_FALSE(std::get<0>(move));
}

TEST(Tpl, DirectTuplePrintingAndStreamPrimitives) {
    // Test stream primitives independently of the public tuple wrapper.
    namespace d = eon::tpl::detail;
    std::ostringstream output;
    d::print(output, std::tuple{42, std::string("a\"b"), 'x'}, std::string_view(";"));
    EXPECT_EQ(output.str(), "42;\"a\\\"b\";'x'");
    std::istringstream match("abc!");
    EXPECT_TRUE(d::accept_range(match, std::string_view("abc")));
    EXPECT_EQ(match.get(), '!');
    std::istringstream mismatch("ax");
    EXPECT_FALSE(d::accept_char(mismatch, 'z'));
    EXPECT_TRUE(mismatch.fail());
    mismatch.clear();
    EXPECT_EQ(mismatch.get(), 'a');
    std::istringstream no_input;
    EXPECT_FALSE(d::accept_char(no_input, 'x'));
    EXPECT_TRUE(no_input.fail());
    std::ostringstream empty_output;
    d::print(empty_output, std::tuple{}, std::string_view(";"));
    EXPECT_TRUE(empty_output.str().empty());
    std::wostringstream wide;
    d::print(wide, std::tuple{7, std::wstring(L"wide")}, std::wstring_view(L";"));
    EXPECT_EQ(wide.str(), L"7;\"wide\"");
}

static_assert(std::same_as<decltype(eon::tpl::detail::quoted_char{std::declval<char &>(), '\''}),
                           eon::tpl::detail::quoted_char<char &>>);

using TupleIndexCallback = decltype([](auto) noexcept {});
static_assert(eon::tpl::tuple_indexes_invocable<TupleIndexCallback, const std::tuple<int> &>);
static_assert(eon::tpl::is_nothrow_tuple_indexes_invocable_v<TupleIndexCallback, std::tuple<int> &&>);
static_assert(std::same_as<eon::tpl::make_tuple_index_sequence<const std::tuple<int> &>, std::index_sequence<0>>);

TEST(Tpl, TuplePublicIoRoundTripsNestedAndQuotedElements) {
    using Tuple = std::tuple<int, std::string, char, std::tuple<int, char>>;
    const Tuple original{42, "hello \"world\"", '\'', {7, '\\'}};
    std::ostringstream out;
    out << eon::tpl::io(original);
    Tuple parsed{};
    std::istringstream input(out.str());
    input >> eon::tpl::io(parsed);
    EXPECT_FALSE(input.fail()) << out.str();
    EXPECT_EQ(parsed, original);
    std::tuple<> empty;
    std::ostringstream eo;
    eo << eon::tpl::io(empty);
    EXPECT_EQ(eo.str(), "()");
    std::istringstream ei("()");
    ei >> eon::tpl::io(empty);
    EXPECT_FALSE(ei.fail());
    std::tuple<int> one{42};
    std::istringstream single("(42)");
    single >> eon::tpl::io(one);
    EXPECT_FALSE(single.fail());
}

TEST(Tpl, TupleCustomSeparatorsAndWideStreams) {
    std::pair<int, int> pair{1, 2}, parsed{};
    for (std::string separator : {";", "::", ""}) {
        // Empty separators round-trip only when element boundaries are unambiguous.
        if (separator.empty()) {
            std::tuple<> t;
            std::ostringstream os;
            os << eon::tpl::io(t, separator);
            EXPECT_EQ(os.str(), "()");
            continue;
        }
        std::ostringstream os;
        os << eon::tpl::io(pair, separator);
        std::istringstream is(os.str());
        is >> eon::tpl::io(parsed, separator);
        EXPECT_FALSE(is.fail());
        EXPECT_EQ(parsed, pair);
    }
    const auto wide = std::tuple{7, std::wstring(L"wide space"), L'x'};
    std::wostringstream os;
    os << eon::tpl::io(wide, L";");
    std::remove_cv_t<decltype(wide)> result{};
    std::wistringstream is(os.str());
    is >> eon::tpl::io(result, L";");
    EXPECT_FALSE(is.fail());
    EXPECT_EQ(result, wide);
    std::wostringstream default_out;
    default_out << eon::tpl::io(wide);
    std::wistringstream default_in(default_out.str());
    default_in >> eon::tpl::io(result);
    EXPECT_FALSE(default_in.fail());
    EXPECT_EQ(result, wide);
}

TEST(Tpl, TupleReadRejectsMalformedAndTruncatedInput) {
    for (auto text : {"", "[1, 2)", "(bad, 2)", "(1; 2)", "(1, 2]", "(1, 2", "(1, ", "("}) {
        SCOPED_TRACE(text);
        std::pair<int, int> result{};
        std::istringstream is(text);
        is >> eon::tpl::io(result);
        EXPECT_TRUE(is.fail());
    }
    std::tuple<int, int> values{9, 9};
    std::istringstream is("(bad, 2)");
    is >> eon::tpl::io(values);
    EXPECT_EQ(std::get<1>(values), 9);
}

template <typename T>
class QuotedCharRegression : public ::testing::Test {};

using StreamCharacters = ::testing::Types<char, wchar_t>;
TYPED_TEST_SUITE(QuotedCharRegression, StreamCharacters);

TYPED_TEST(QuotedCharRegression, RoundTripsEscapesWhitespaceAndNull) {
    using C = TypeParam;
    for (C c : {C('a'), C('\''), C('\\'), C(' '), C{}, C('\n')}) {
        std::basic_ostringstream<C> os;
        os << eon::tpl::detail::quoted(c);
        C parsed = C('x');
        std::basic_istringstream<C> is(os.str());
        is >> eon::tpl::detail::quoted(parsed);
        EXPECT_FALSE(is.fail());
        EXPECT_EQ(parsed, c);
    }
}

TEST(Tpl, QuotedCharFailureLeavesDestinationUntouched) {
    for (auto text : {"", "a", "'", "''", "'ab'", "'a", "'\\"}) {
        SCOPED_TRACE(text);
        char c = 'x';
        std::istringstream is(text);
        is >> eon::tpl::detail::quoted(c);
        EXPECT_TRUE(is.fail());
        EXPECT_EQ(c, 'x');
    }
    const char c = 'q';
    std::ostringstream out;
    out << eon::tpl::detail::quoted(c);
    EXPECT_EQ(out.str(), "'q'");
    out << eon::tpl::detail::quoted('z');
    EXPECT_EQ(out.str(), "'q''z'");
}

// A non-seekable streambuf with a get area supporting putback of consumed
// bytes.
class NonSeekableBuffer : public std::streambuf {
    std::string data;

public:
    explicit NonSeekableBuffer(std::string text) : data(std::move(text)) {
        setg(data.data(), data.data(), data.data() + data.size());
    }
};

TEST(Tpl, SeparatorRollbackHandlesMismatchEofAndNonSeekableStreams) {
    for (auto text : {"", "a", "ax!", "abx!"}) {
        SCOPED_TRACE(text);
        std::istringstream seekable(text);
        NonSeekableBuffer buffer(text);
        std::istream nonseekable(&buffer);
        for (std::istream * is : {static_cast<std::istream *>(&seekable), &nonseekable}) {
            EXPECT_FALSE(eon::tpl::detail::accept_range(*is, std::string_view("abc")));
            EXPECT_TRUE(is->fail());
            EXPECT_FALSE(is->bad());
            is->clear();
            std::string remaining;
            std::getline(*is, remaining);
            EXPECT_EQ(remaining, text);
        }
    }
    std::istringstream is("abc!");
    std::forward_list<char> delimiter{'a', 'b', 'c'};
    EXPECT_TRUE(eon::tpl::detail::accept_range(is, delimiter));
    EXPECT_EQ(is.get(), '!');
    std::istringstream empty("x");
    EXPECT_TRUE(eon::tpl::detail::accept_range(empty, std::string_view{}));
    EXPECT_EQ(empty.get(), 'x');
    std::istringstream delimiter_stream("abc");
    auto input_range = std::views::istream<char>(delimiter_stream);
    std::istringstream input("abc!");
    EXPECT_TRUE(eon::tpl::detail::accept_range(input, input_range));
    EXPECT_EQ(input.get(), '!');
}

class NoPutbackBuffer : public std::streambuf {
    std::string text = "ax";
    std::size_t position = 0;

protected:
    int_type underflow() override {
        return position == text.size() ? traits_type::eof() : traits_type::to_int_type(text[position]);
    }

    int_type uflow() override {
        auto c = underflow();
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            ++position;
        }
        return c;
    }
};

TEST(Tpl, RollbackFailureSetsBadbit) {
    NoPutbackBuffer buffer;
    std::istream is(&buffer);
    EXPECT_FALSE(eon::tpl::detail::accept_range(is, std::string_view("ab")));
    EXPECT_TRUE(is.fail());
    EXPECT_TRUE(is.bad());
}

TEST(Tpl, SingleCharacterMismatchDoesNotConsumeInput) {
    std::istringstream is("x");
    EXPECT_FALSE(eon::tpl::detail::accept_char(is, 'a'));
    is.clear();
    EXPECT_EQ(is.get(), 'x');
    std::istringstream eof;
    EXPECT_FALSE(eon::tpl::detail::accept_char(eof, 'x'));
    EXPECT_FALSE(eon::tpl::detail::expect_char(eof, char(-1)));
}

TEST(Tpl, SeparatorRollbackPrecedesStreamFailureException) {
    for (auto text : {"ax!", "a"}) {
        std::istringstream is(text);
        is.exceptions(std::ios_base::failbit | std::ios_base::eofbit);
        EXPECT_THROW(eon::tpl::detail::accept_range(is, std::string_view("abc")), std::ios_base::failure);
        is.exceptions(std::ios_base::goodbit);
        is.clear();
        std::string remaining;
        std::getline(is, remaining);
        EXPECT_EQ(remaining, text);
    }
}
