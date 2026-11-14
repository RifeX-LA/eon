#include "test.hpp"
#include <cfenv>
#include <eon/spark.hpp>
#include <eon/spark/arithmetic.hpp>
#include <eon/spark/compare.hpp>
#include <forward_list>
namespace s = eon::spark;

struct ThrowingProjection {
    int operator()(int) const {
        throw std::runtime_error("projection");
    }
};

struct NoThrowIdentity {
    int operator()(int value) const noexcept {
        return value;
    }
};

using ThrowingProjected = s::proj<ThrowingProjection, NoThrowIdentity>;
static_assert(!noexcept(std::declval<ThrowingProjected const &>()(1)));
static_assert(noexcept(s::proj{NoThrowIdentity{}, NoThrowIdentity{}}(1)));

TEST(Spark, ProjectionExceptionsPropagate) {
    const ThrowingProjected projected{ThrowingProjection{}, NoThrowIdentity{}};
    EXPECT_THROW(projected(1), std::runtime_error);
}

TEST(Spark, ComparingOppositeExtremesDoesNotRaiseFloatingOverflow) {
    std::fenv_t previous;
    ASSERT_EQ(std::fegetenv(&previous), 0);
    std::feclearexcept(FE_ALL_EXCEPT);
    volatile double max = std::numeric_limits<double>::max();
    EXPECT_FALSE(s::equal{}(max, -max));
    EXPECT_EQ(std::fetestexcept(FE_OVERFLOW), 0);
    std::fesetenv(&previous);
}

TEST(Spark, InfinityComparesEqualToItself) {
    EXPECT_TRUE(s::equal{}(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()));
}

template <typename T>
constexpr bool arithmetic() {
    constexpr T lo = std::numeric_limits<T>::lowest(), hi = std::numeric_limits<T>::max();
    if (s::add(T{0})(lo) != lo || s::sub(T{0})(hi) != hi || s::mult(T{1})(lo) != lo) {
        return false;
    }
    if (s::divides(T{1})(hi) != hi || s::remainder(T{1})(lo) != 0) {
        return false;
    }
    if (s::add(T{1})(T{2}) != 3 || s::sub(T{2})(T{5}) != 3 || s::mult(T{3})(T{2}) != 6) {
        return false;
    }
    if (s::bit_and(T{3})(T{6}) != 2 || s::bit_or(T{3})(T{6}) != 7 || s::bit_xor(T{3})(T{6}) != 5) {
        return false;
    }
    return s::lshift(1)(T{1}) == 2 && s::rshift(1)(T{2}) == 1;
}

static_assert(arithmetic<int>() && arithmetic<unsigned>() && arithmetic<long long>() &&
              arithmetic<unsigned long long>());
static_assert(arithmetic<signed char>() && arithmetic<unsigned char>() && arithmetic<short>() &&
              arithmetic<unsigned short>());
static_assert(arithmetic<long>() && arithmetic<unsigned long>());

template <typename T, class U>
constexpr bool compare_boundaries() {
    for (T a : {std::numeric_limits<T>::lowest(), T{0}, T{1}, std::numeric_limits<T>::max()}) {
        for (U b : {std::numeric_limits<U>::lowest(), U{0}, U{1}, std::numeric_limits<U>::max()}) {
            if (s::equal{}(a, b) != std::cmp_equal(a, b)) {
                return false;
            }
            if (s::not_equal{}(a, b) != std::cmp_not_equal(a, b)) {
                return false;
            }
            if (s::less{}(a, b) != std::cmp_less(a, b)) {
                return false;
            }
            if (s::less_equal{}(a, b) != std::cmp_less_equal(a, b)) {
                return false;
            }
            if (s::greater{}(a, b) != std::cmp_greater(a, b)) {
                return false;
            }
            if (s::greater_equal{}(a, b) != std::cmp_greater_equal(a, b)) {
                return false;
            }
        }
    }
    return true;
}

static_assert(compare_boundaries<int, unsigned>() && compare_boundaries<unsigned, int>());
static_assert(compare_boundaries<long long, unsigned long long>() &&
              compare_boundaries<unsigned long long, long long>());
static_assert(compare_boundaries<int, unsigned long long>() && compare_boundaries<long long, unsigned>());
static_assert(s::add(1u)(std::numeric_limits<unsigned>::max()) == 0u);
static_assert(s::sub(1u)(0u) == std::numeric_limits<unsigned>::max());
static_assert(s::mult(2u)(std::numeric_limits<unsigned>::max()) == std::numeric_limits<unsigned>::max() - 1);
static_assert(s::lshift(std::numeric_limits<unsigned>::digits - 1)(1u) ==
              (std::numeric_limits<unsigned>::max() / 2 + 1));
static_assert(s::rshift(std::numeric_limits<unsigned>::digits - 1)(std::numeric_limits<unsigned>::max()) == 1);
static_assert(s::divides(2)(-7) == -3 && s::remainder(2)(-7) == -1);
static_assert(s::less{}(-1, 0u) && s::greater{}(0u, -1));
static_assert(!s::equal{}(-1, std::numeric_limits<unsigned>::max()));
static_assert(s::less_equal{}(0, 0u) && s::greater_equal{}(0u, 0));
static_assert(s::not_equal{}(-1, 1u));
static_assert(s::div(3)(-6) && s::ndiv(3)(-7));
static_assert(s::static_caster<int>(3.9) == 3 && !s::has_value(0));
enum class Code : unsigned { max = std::numeric_limits<unsigned>::max() };
static_assert(s::to_underlying(Code::max) == std::numeric_limits<unsigned>::max());
static_assert(s::get<1>(std::pair{1, 2}) == 2);
static_assert(s::always(4)(1, 2, 3) == 4);
static_assert(s::proj{[](int x) { return x * 2; }, std::plus{}}(2, 3) == 10);
static_assert([] {
    int x = 2;
    return &s::assign(s::add(3))(x) == &x && x == 5;
}());
static_assert([] {
    int a[]{1, 2, 3};
    return s::front(a) == 1 && s::back(a) == 3 && s::indirect(s::addressof(a[1])) == 2;
}());

struct Base {
    virtual ~Base() = default;
};

struct Child : Base {};

struct Other : Base {};

struct MemberGet {
    template <std::size_t I>
    constexpr int get() const noexcept {
        return int(I) + 10;
    }
};

namespace custom {
struct AdlGet {};

template <std::size_t I>
constexpr int get(AdlGet) {
    return int(I) + 20;
}
} // namespace custom

static_assert(s::get<2>(MemberGet{}) == 12 && noexcept(s::get<2>(MemberGet{})));
static_assert(s::get<2>(custom::AdlGet{}) == 22 && !noexcept(s::get<2>(custom::AdlGet{})));
static_assert(std::same_as<decltype(s::get<0>(std::declval<std::tuple<int> &>())), int &>);
static_assert(std::same_as<decltype(s::get<0>(std::declval<const std::tuple<int> &>())), const int &>);
static_assert(std::same_as<decltype(s::get<0>(std::declval<std::tuple<int> &&>())), int &&>);

TEST(Spark, SignedUnsignedComparisonMatrix) {
    for (int a = -32; a <= 32; ++a) {
        for (int b = 0; b <= 32; ++b) {
            SCOPED_TRACE(::testing::Message() << "a=" << a << ", b=" << b);
            EXPECT_EQ(s::equal{}(+a, static_cast<unsigned>(b)), a == b);
            EXPECT_EQ(s::less{}(+a, static_cast<unsigned>(b)), a < b);
            EXPECT_EQ(s::greater{}(+a, static_cast<unsigned>(b)), a > b);
        }
    }
}

TEST(Spark, FloatingPointTolerance) {
    const double eps = std::numeric_limits<double>::epsilon();
    EXPECT_TRUE(s::equal{}(0., +eps));
    EXPECT_FALSE(s::equal{}(0., 2 * eps));
    EXPECT_TRUE(s::not_equal{}(0., 2 * eps));
    EXPECT_FALSE(s::less{}(0., +eps));
    EXPECT_TRUE(s::less{}(0., 2 * eps));
    EXPECT_TRUE(s::less_equal{}(+eps, 0.));
    EXPECT_TRUE(s::greater{}(2 * eps, 0.));
    EXPECT_TRUE(s::greater_equal{}(0., +eps));
    EXPECT_TRUE(s::equal{}(-0., 0.));
    double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(s::equal{}(+nan, 0.));
    EXPECT_FALSE(s::less{}(+nan, 0.));
    EXPECT_FALSE(s::greater{}(+nan, 0.));
    EXPECT_TRUE(s::less{}(std::string("a"), std::string("b")));
}

TEST(Spark, CastsHashAndInvocation) {
    Child c;
    Other o;
    Base * p = &c;
    EXPECT_EQ(s::dynamic_caster<Child *>(p), &c);
    EXPECT_EQ(s::dynamic_caster<Child *>(static_cast<Base *>(&o)), nullptr);
    EXPECT_EQ(s::dynamic_caster<Child *>(static_cast<Base *>(nullptr)), nullptr);
    EXPECT_THROW(([&] { (void)s::dynamic_caster<Child &>(o); })(), std::bad_cast);
    int n = 3;
    const int * cp = &n;
    EXPECT_EQ(s::const_caster<int *>(cp), &n);
    EXPECT_EQ(s::reinterpret_caster<void *>(&n), static_cast<void *>(&n));
    EXPECT_EQ(s::hash(n), std::hash<int>{}(n));
    EXPECT_EQ(s::invoke([](int x) { return x + 1; }, 3), 4);
}

TEST(Spark, BackForDifferentRangeCategories) {
    std::forward_list<int> list{1, 2, 3};
    EXPECT_EQ(s::back(list), 3);
    auto sized = std::ranges::subrange(list.begin(), list.end(), 3);
    EXPECT_EQ(s::back(sized), 3);
    auto noncommon = std::views::iota(0) | std::views::take(4);
    EXPECT_EQ(s::back(noncommon), 3);
}

TEST(Spark, AddressAndIndirectViewsPreserveReferences) {
    std::vector<int> v{1, 2, 3};
    auto cast = v | s::views::static_caster<double>;
    EXPECT_TRUE(std::ranges::equal(cast, std::array{1., 2., 3.}));
    auto addresses = v | s::views::addressof;
    EXPECT_EQ(*addresses.begin(), &v[0]);
    auto indirect = addresses | s::views::indirect;
    *indirect.begin() = 9;
    EXPECT_EQ(v[0], 9);
}

TEST(Spark, HasValueViewFiltersEmptyOptionals) {
    std::vector<std::optional<int>> opts{1, std::nullopt, 3};
    EXPECT_TRUE(std::ranges::equal(opts | s::views::has_value | s::views::indirect, std::array{1, 3}));
}

TEST(Spark, NullableRangeReturnsEmptyForNullPointer) {
    std::vector<int> v{1, 2, 3};
    std::vector<int> * vp = &v;
    EXPECT_TRUE(std::ranges::equal(s::range_from_nullable(vp), v));
    vp = nullptr;
    EXPECT_TRUE(std::ranges::empty(s::range_from_nullable(vp)));
}

TEST(Spark, CastingAnEmptyRangeIsEmpty) {
    std::vector<int> empty;
    EXPECT_TRUE(std::ranges::empty(empty | s::views::static_caster<double>));
}

TEST(Spark, GetViewPreservesElementReferences) {
    std::vector<std::pair<int, int>> pairs{{1, 2}, {3, 4}};
    EXPECT_TRUE(std::ranges::equal(pairs | s::views::get<0>, std::array{1, 3}));
    EXPECT_EQ(s::key(pairs[0]), 1);
    EXPECT_EQ(s::value(pairs[0]), 2);
    auto values = pairs | s::views::get<1>;
    *values.begin() = 7;
    EXPECT_EQ(pairs[0].second, 7);
}

TEST(Spark, FrontAndBackViewsSelectRangeEndpoints) {
    std::vector<std::vector<int>> nested{{1, 2}, {3}};
    EXPECT_TRUE(std::ranges::equal(nested | s::views::front, std::array{1, 3}));
    EXPECT_TRUE(std::ranges::equal(nested | s::views::back, std::array{2, 3}));
}

TEST(Spark, InvokeViewCallsEachFunction) {
    std::array<std::function<int()>, 2> functions{[] { return 1; }, [] { return 2; }};
    EXPECT_TRUE(std::ranges::equal(functions | s::views::invoke, std::array{1, 2}));
}

TEST(Spark, UnderlyingViewConvertsEnums) {
    std::array codes{Code::max};
    EXPECT_EQ(*(codes | s::views::to_underlying).begin(), std::numeric_limits<unsigned>::max());
}

TEST(Spark, HashViewUsesStandardHash) {
    std::array nums{1, 2};
    auto hashes = nums | s::views::hash;
    EXPECT_EQ(*hashes.begin(), std::hash<int>{}(1));
}

TEST(Spark, DynamicCastViewReturnsNullForWrongType) {
    Child c;
    Other o;
    std::array<Base *, 3> bases{&c, &o, nullptr};
    auto children = bases | s::views::dynamic_caster<Child *>;
    auto it = children.begin();
    EXPECT_EQ(*it, &c);
    EXPECT_EQ(*++it, nullptr);
    EXPECT_EQ(*++it, nullptr);
}

TEST(Spark, ConstCastViewPreservesAddress) {
    int n = 3;
    std::array<const int *, 1> const_ptrs{&n};
    EXPECT_EQ(*s::views::const_caster<int *>(const_ptrs).begin(), &n);
}

TEST(Spark, ReinterpretCastViewPreservesAddress) {
    int n = 3;
    std::array<int *, 1> ptrs{&n};
    EXPECT_EQ(*(ptrs | s::views::reinterpret_caster<void *>).begin(), static_cast<void *>(&n));
}

TEST(Spark, NullableRangeViewHandlesNullElements) {
    std::vector<int> v{1, 2, 3};
    std::array<std::vector<int> *, 2> ranges{&v, nullptr};
    auto nullable_ranges = ranges | s::views::range_from_nullable;
    EXPECT_TRUE(std::ranges::equal(*nullable_ranges.begin(), v));
    EXPECT_TRUE(std::ranges::empty(*++nullable_ranges.begin()));
}

TEST(Spark, FilteringAllAbsentValuesProducesEmptyRange) {
    std::array<std::optional<int>, 2> absent{};
    EXPECT_TRUE(std::ranges::empty(absent | s::views::has_value));
}

TEST(Spark, StubAcceptsAnyArguments) {
    s::stub();
    s::stub(1, "anything");
}

static_assert(s::zero(0) && !s::zero(1) && s::non_zero(-1));
static_assert(s::negative(std::numeric_limits<int>::min()));
static_assert(s::positive(std::numeric_limits<unsigned>::max()));
static_assert(s::non_negative(0) && s::non_positive(0));
static_assert(s::eq(3)(3) && s::neq(3)(4) && s::lt(3)(2));
static_assert(s::le(3)(3) && s::gt(3)(4) && s::ge(3)(3));
static_assert([] {
    int a = -1;
    const unsigned b = 0;
    return s::less{}(a, b) && s::greater{}(b, a) && !s::equal{}(a, b) && s::less_equal{}(a, b) &&
           s::greater_equal{}(b, a) && s::not_equal{}(a, b);
}());
static_assert(s::less{}(char(-1), std::numeric_limits<unsigned>::max()));
static_assert(s::equal{}(true, 1) && s::less{}(false, true));
static_assert(s::equal{}(u'x', U'x') && s::neq(L'x')(L'y'));

template <std::floating_point T>
constexpr bool floating_sign_predicates() {
    for (T value : {T{-1}, T{0}, -T{0}, T{1}, std::numeric_limits<T>::min(), std::numeric_limits<T>::lowest(),
                    std::numeric_limits<T>::max(), std::numeric_limits<T>::epsilon() / T{2}}) {
        if (s::zero(value) != (value == 0) || s::non_zero(value) != (value != 0) || s::negative(value) != (value < 0) ||
            s::positive(value) != (value > 0) || s::non_negative(value) != (value >= 0) ||
            s::non_positive(value) != (value <= 0)) {
            return false;
        }
    }
    return true;
}

static_assert(floating_sign_predicates<float>());
static_assert(floating_sign_predicates<double>());
static_assert(floating_sign_predicates<long double>());
static_assert(s::equal{}(1, 1.0) && s::equal{}(1.0, 1));
static_assert(s::not_equal{}(1, 1.5) && s::not_equal{}(1.5, 1));
static_assert(s::less{}(1, 1.5) && s::less{}(1.5, 2));
static_assert(s::greater{}(2, 1.5) && s::greater{}(1.5, 1));
static_assert(s::less_equal{}(1, 1.0) && s::less_equal{}(1.0, 1));
static_assert(s::greater_equal{}(1, 1.0) && s::greater_equal{}(1.0, 1));

TEST(Spark, LvalueComparisonsPreserveSpecializedSemantics) {
    const int negative = -1;
    unsigned maximum = std::numeric_limits<unsigned>::max();
    EXPECT_TRUE(s::less{}(negative, maximum));
    EXPECT_FALSE(s::equal{}(negative, maximum));

    const double one = 1.0;
    double near_one = one + std::numeric_limits<double>::epsilon();
    EXPECT_TRUE(s::equal{}(one, near_one));
    EXPECT_FALSE(s::less{}(one, near_one));
}

TEST(Spark, FloatingSignPredicatesHandleInfinitiesAndNan) {
    double const infinity = std::numeric_limits<double>::infinity();
    double const nan = std::numeric_limits<double>::quiet_NaN();
    for (double value : {infinity, -infinity, nan}) {
        EXPECT_EQ(s::zero(value), value == 0);
        EXPECT_EQ(s::non_zero(value), value != 0);
        EXPECT_EQ(s::negative(value), value < 0);
        EXPECT_EQ(s::positive(value), value > 0);
        EXPECT_EQ(s::non_negative(value), value >= 0);
        EXPECT_EQ(s::non_positive(value), value <= 0);
    }
}

TEST(Spark, ConstCasterPipePreservesAddressAndMutability) {
    int value = 7;
    std::array<const int *, 1> pointers{&value};
    auto cast = pointers | s::views::const_caster<int *>;
    ASSERT_EQ(*cast.begin(), &value);
    **cast.begin() = 9;
    EXPECT_EQ(value, 9);
    auto composed = s::views::const_caster<int *> | s::views::indirect;
    EXPECT_EQ(*(pointers | composed).begin(), 9);
    std::array<const int *, 0> empty{};
    EXPECT_TRUE(std::ranges::empty(empty | composed));
}

TEST(Spark, FloatingSpecialValuesAndComplements) {
    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();
    for (double a : {0., -0., 1., -1., inf, -inf, nan}) {
        for (double b : {0., -0., 1., -1., inf, -inf, nan}) {
            SCOPED_TRACE(::testing::Message() << a << ", " << b);
            EXPECT_EQ(s::not_equal{}(a, b), !s::equal{}(a, b));
            EXPECT_EQ(s::equal{}(a, b), a == b);
            EXPECT_EQ(s::less{}(a, b), a < b);
            EXPECT_EQ(s::less_equal{}(a, b), a <= b);
            EXPECT_EQ(s::greater{}(a, b), a > b);
            EXPECT_EQ(s::greater_equal{}(a, b), a >= b);
        }
    }
}

template <typename T>
class FloatingScaleRegression : public ::testing::Test {};

using Floats = ::testing::Types<float, double, long double>;
TYPED_TEST_SUITE(FloatingScaleRegression, Floats);

TYPED_TEST(FloatingScaleRegression, RelativeToleranceAtLargeAndExtremeValues) {
    using T = TypeParam;
    for (T value : {T{1}, T{1024}, std::numeric_limits<T>::max()}) {
        T adjacent = std::nextafter(value, T{});
        EXPECT_TRUE(s::equal{}(value, adjacent));
        EXPECT_FALSE(s::less{}(adjacent, value));
        EXPECT_FALSE(s::greater{}(value, adjacent));
        EXPECT_TRUE(s::less_equal{}(value, adjacent));
        EXPECT_TRUE(s::greater_equal{}(adjacent, value));
    }
    T max = std::numeric_limits<T>::max();
    EXPECT_FALSE(s::equal{}(max, -max));
    EXPECT_TRUE(s::less{}(-max, max));
    T eps = std::numeric_limits<T>::epsilon();
    EXPECT_TRUE(s::equal{}(T{}, eps));
    EXPECT_FALSE(s::equal{}(T{}, T{2} * eps));
}
