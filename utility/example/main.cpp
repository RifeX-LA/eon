#include <vector>
#include <algorithm>
#include <concepts>
#include <iostream>

#include <eon/utility.hpp>

bool foo(std::predicate<int> auto predicate) {
    std::vector v{1, 2, 3};
    // eon::pass_fn() will choose when better to pass by copy and when better to pass by reference (std::reference_wrapper)
    return std::ranges::any_of(v, eon::pass_fn(predicate));
}

int main() {
    // expensive to copy that
    auto expensive_predicate = [param1 = 10, param2 = 42 /* etc */](int value) {
        //...
        return true;
    };

    // cheap to copy that
    auto cheap_predicate = [](int value) {
        return value == 42;
    };

    std::cout << foo(eon::pass_fn(expensive_predicate)) << '\n'; // will be passed by std::reference_wrapper to std::ranges::any_of
    std::cout << foo(cheap_predicate) << '\n'; // will be passed by copy to std::ranges::any_of

    return 0;
}