#include <eon/lifetime.hpp>

int main() {
    eon::lifetime l1;
    eon::lifetime l2(42);

    l1 == l2;
    l1 = l2;
    l1 = std::move(l2);

    std::println("{:b}", l1);
    std::println("Total: {}. Alive: {}", eon::lifetime::total(), eon::lifetime::alive());

    return 0;
}
