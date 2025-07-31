#include <eon/concepts.hpp>

template <typename T>
struct base {

};

struct derived : base<int> {

};


int main() {
    static_assert(eon::specialization_of<base<int>, base>);
    static_assert(!eon::specialization_of<derived, base>);
    static_assert(!eon::specialization_of<int, base>);

    static_assert(eon::derived_from_specialization_of<base<int>, base>);
    static_assert(eon::derived_from_specialization_of<derived, base>);
    static_assert(!eon::derived_from_specialization_of<int, base>);

    return 0;
}