#include <functional>
#include <iostream>
#include <vector>
#include <memory>

#include <eon/nullable.hpp>

struct widget {
    int a = 42;
    std::optional<int> b = 42;
    std::unique_ptr<int> c;

    [[nodiscard]] int foo() const {
        return a;
    }

    [[nodiscard]] std::optional<int> const & bar(int) const {
        return b;
    }
};

void bar(widget const & w) {
    std::cout << w.a;
}

void func(int i) {
    std::cout << i;
}

void foo() {
    return void{};
}

int main() {
    std::optional<widget> opt = widget{};

    // if (opt) {
    //     if (auto const value = opt->foo(42))
    //         std::cout << *value;
    //     else
    //         std::cout << "empty";
    // }
    // else
    //     std::cout << "empty";

    if (auto const value = eon::invoke_nullable_ref(&widget::bar, opt, 42))
        std::cout << *value << std::endl;
    else
        std::cout << "empty\n";

    return 0;
}