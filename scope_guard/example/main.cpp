#include <iostream>

#include <eon/scope_guard.hpp>

int main() {
    std::cout << "Hello";
    {
        eon::scope_guard scope_guard([] {
           std::cout << ", World\n";
        });
    }
    std::cout << "Goodbye\n";

    return 0;
}