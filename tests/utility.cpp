#include "test.hpp"
#include <eon/utility.hpp>

struct Big {
    int data[20]{};
};

struct Nontrivial {
    Nontrivial() = default;

    Nontrivial(Nontrivial const &) {
    }
};

static_assert(std::same_as<decltype(eon::pass_fn(std::declval<int &>())), int>);
static_assert(std::same_as<decltype(eon::pass_fn(std::declval<Big &>())), std::reference_wrapper<Big>>);
static_assert(std::same_as<decltype(eon::pass_fn(std::declval<Nontrivial &>())), std::reference_wrapper<Nontrivial>>);
static_assert(std::same_as<decltype(eon::unwrap_ref(std::declval<const int &>())), const int &>);
static_assert(std::same_as<decltype(eon::unwrap_ref(1)), int &&>);
static_assert(std::same_as<decltype(eon::unwrap_ptr(std::declval<const int *>())), const int &>);
static_assert(std::same_as<decltype(eon::unwrap_ptr(std::declval<int &>())), int &>);
static_assert([] {
    int n = 4;
    return &eon::unwrap_ptr(&n) == &n && &eon::unwrap_ref(std::ref(n)) == &n;
}());
static_assert([] {
    int n = 0;
    eon::for_each_type<int, char, double>([&](auto) { ++n; });
    return n == 3;
}());
static_assert([] {
    int n = 0;
    eon::for_each_type<>([&](auto) { ++n; });
    return n == 0;
}());
static_assert(eon::invocable_for_types<decltype([](auto) {}), int, double>);
static_assert(eon::is_nothrow_invocable_for_types_v<decltype([](auto) noexcept {}), int>);
static_assert(!eon::is_nothrow_invocable_for_types_v<decltype([](auto) {}), int>);

TEST(Utility, ReferenceAndCopySemantics) {
    Big b;
    auto ref = eon::pass_fn(b);
    EXPECT_EQ(&ref.get(), &b);
    const int n = 7;
    EXPECT_EQ(&eon::unwrap_ref(std::cref(n)), &n);
    EXPECT_TRUE(noexcept(eon::unwrap_ref(n)));
    EXPECT_TRUE(noexcept(eon::pass_fn(b)));
    int value = 3;
    auto copied = eon::pass_fn(value);
    copied = 9;
    EXPECT_EQ(value, 3);
    EXPECT_EQ(copied, 9);
}

TEST(Utility, EnumeratesTupleAndVariantTypes) {
    std::vector<std::size_t> sizes;
    eon::for_each_tuple_element<std::tuple<char, int, double>>(
        [&](auto t) { sizes.push_back(sizeof(typename decltype(t)::type)); });
    EXPECT_EQ(sizes, std::vector<std::size_t>({sizeof(char), sizeof(int), sizeof(double)}));
    sizes.clear();
    eon::for_each_variant_alternative<std::variant<char, int>>(
        [&](auto t) { sizes.push_back(sizeof(typename decltype(t)::type)); });
    EXPECT_EQ(sizes, std::vector<std::size_t>({sizeof(char), sizeof(int)}));
}

static_assert(std::same_as<eon::make_type_pack_t<std::tuple<>>, eon::type_pack<>>);
static_assert(std::same_as<eon::make_type_pack_t<std::tuple<int, char>>, eon::type_pack<int, char>>);
static_assert(std::same_as<eon::make_type_pack_t<const std::tuple<int, char>>, eon::type_pack<const int, const char>>);
static_assert(
    std::same_as<eon::make_type_pack_t<const std::variant<int, char>>, eon::type_pack<const int, const char>>);
static_assert(std::same_as<eon::make_type_pack_t<const eon::type_pack<int, char>>, eon::type_pack<int, char>>);
using AnyType = decltype([](auto) noexcept {});
using OnlyInt = decltype([](std::type_identity<int>) noexcept {});
static_assert(eon::invocable_for_packed_types<AnyType, std::tuple<int, char>>);
static_assert(!eon::invocable_for_packed_types<OnlyInt, std::tuple<int, char>>);
static_assert(!eon::invocable_for_packed_types<AnyType, int>);
static_assert(!eon::invocable_for_packed_types<AnyType, std::tuple<int> &>);
static_assert(eon::invocable_for_packed_types<OnlyInt, eon::type_pack<>>);
static_assert(eon::is_nothrow_invocable_for_packed_types_v<AnyType, std::variant<int, char>>);
static_assert(!eon::is_nothrow_invocable_for_packed_types_v<decltype([](auto) {}), std::tuple<int>>);
static_assert(noexcept(eon::for_each_type_in_pack<std::tuple<int>>(AnyType{})));
static_assert([] {
    int n = 0;
    eon::for_each_type_in_pack<std::tuple<char, int, double>>([&](auto) { ++n; });
    eon::for_each_type_in_pack<eon::type_pack<>>([&](auto) { ++n; });
    return n == 3;
}());

TEST(Utility, TypePacksPreserveOrderDuplicatesAndExceptions) {
    std::vector<std::size_t> sizes;
    auto append = [&](auto t) { sizes.push_back(sizeof(typename decltype(t)::type)); };
    eon::for_each_type_in_pack<std::tuple<char, int, char>>(append);
    EXPECT_EQ(sizes, (std::vector<std::size_t>{sizeof(char), sizeof(int), sizeof(char)}));
    sizes.clear();
    eon::for_each_type_in_pack<std::variant<char, double>>(append);
    EXPECT_EQ(sizes, (std::vector<std::size_t>{sizeof(char), sizeof(double)}));
    sizes.clear();
    eon::for_each_type_in_pack<eon::type_pack<int>>(append);
    EXPECT_EQ(sizes, std::vector<std::size_t>{sizeof(int)});
    EXPECT_THROW(eon::for_each_type_in_pack<std::tuple<int>>([](auto) { throw std::runtime_error("test"); }),
                 std::runtime_error);
}
