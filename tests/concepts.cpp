#include "test.hpp"
#include <deque>
#include <eon/concepts.hpp>
#include <list>
#include <optional>
#include <variant>

using namespace eon;
static_assert(decayed<int> && !decayed<const int> && !decayed<int &> && !decayed<int[2]>);
static_assert(without_cv<int &> && !without_cv<const int>);
static_assert(without_ref<const int> && !without_ref<int &&>);
static_assert(without_cvref<int> && !without_cvref<const int &>);
static_assert(ref_or_ptr<int *> && ref_or_ptr<int &> && ref_or_ptr<int &&> && !ref_or_ptr<int>);
static_assert(exists_in<int, char, int, double> && !exists_in<int> && !exists_in<int, const int>);
static_assert(character<char> && character<const unsigned char &> && character<signed char>);
static_assert(character<wchar_t> && character<char8_t> && character<char16_t> && character<char32_t>);
static_assert(!character<bool> && !character<int> && !character<char *>);
static_assert(indirectable<int *> && indirectable<std::optional<int>> && !indirectable<void *> && !indirectable<int>);
using Fn = decltype([](int) -> int { return 0; });
static_assert(invocable_r<Fn, int, int> && !invocable_r<Fn, long, int>);
static_assert(invocable_conv_r<Fn, long, int> && !invocable_r<Fn, int>);
static_assert(specialization_of<const std::vector<int> &, std::vector> && !specialization_of<int, std::vector>);

struct Derived : std::vector<int> {};

struct Private : private std::vector<int> {};

static_assert(derived_from_specialization_of<Derived, std::vector>);
static_assert(!derived_from_specialization_of<Private, std::vector>);
static_assert(tuple_like<std::tuple<>> && tuple_like<const std::tuple<int> &>);
static_assert(pair_like<std::pair<int, char>> && !pair_like<std::tuple<int>> && !tuple_like<int>);
static_assert(range_of<std::vector<int>, int> && !range_of<std::vector<int>, const int>);
static_assert(input_range_of<std::list<int>, int> && forward_range_of<std::list<int>, int>);
static_assert(bidirectional_range_of<std::list<int>, int> && !random_access_range_of<std::list<int>, int>);
static_assert(random_access_range_of<std::deque<int>, int> && !contiguous_range_of<std::deque<int>, int>);
static_assert(contiguous_range_of<std::array<int, 0>, int>);
static_assert(view_of<std::ranges::empty_view<int>, int> && !view_of<std::vector<int>, int>);
static_assert(range_of_characters<std::string> && !range_of_characters<std::vector<int>>);
static_assert(character_ptr<const char *> && !character_ptr<int *>);
static_assert(character_array<char[]> && bounded_character_array<char[1]> && unbounded_character_array<char[]>);
static_assert(!bounded_character_array<char[]> && !unbounded_character_array<char[1]>);
static_assert(string_like<std::wstring> && string_like<std::u8string_view> && string_like<const char *>);
static_assert(sized_string_like<char[1]> && !sized_string_like<char *>);
static_assert(unsized_string_like<char *> && unsized_string_like<char[]> && !unsized_string_like<std::string>);

TEST(Concepts, CompileTimeContracts) {
    SUCCEED();
}
