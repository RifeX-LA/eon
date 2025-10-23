#include <array>

#include <eon/concepts.hpp>


template <typename T>
struct base {

};

struct derived : base<int> {

};

namespace {

    struct widget {
    private:
        int a;
        double b;
    public:
        int & get_a() {
            return a;
        }

        int const & get_a() const {
            return a;
        }

        double & get_b() {
            return b;
        }

        double const & get_b() const {
            return b;
        }
    };

    template <std::size_t I>
    std::tuple_element_t<I, widget> & get(widget & w) {
        if constexpr (I == 0) {
            return w.get_a();
        }
        else {
            return w.get_b();
        }
    }

    template <std::size_t I>
    std::tuple_element_t<I, widget> const & get(widget const & w) {
        if constexpr (I == 0) {
            return w.get_a();
        }
        else {
            return w.get_b();
        }
    }

}

template <>
struct std::tuple_size<widget> : std::integral_constant<std::size_t, 2> {};

template <>
struct std::tuple_element<0, widget> : std::type_identity<int> {};

template <>
struct std::tuple_element<1, widget> : std::type_identity<double> {};


int main() {
    static_assert(eon::specialization_of<base<int>, base>);
    static_assert(!eon::specialization_of<derived, base>);
    static_assert(!eon::specialization_of<int, base>);

    static_assert(eon::derived_from_specialization_of<base<int>, base>);
    static_assert(eon::derived_from_specialization_of<derived, base>);
    static_assert(!eon::derived_from_specialization_of<int, base>);

    static_assert(!eon::tuple_like<int>);
    static_assert(!eon::pair_like<int>);
    static_assert(!eon::tuple_like<derived>);
    static_assert(eon::tuple_like<std::tuple<>>);
    static_assert(eon::tuple_like<std::tuple<int>>);
    static_assert(eon::tuple_like<std::tuple<double, int>>);
    static_assert(eon::pair_like<std::tuple<double, int>>);
    static_assert(eon::pair_like<std::pair<int, int>>);
    static_assert(eon::tuple_like<std::array<int, 42>>);
    static_assert(eon::pair_like<widget>);

    return 0;
}