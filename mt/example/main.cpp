#include <iostream>

#include <eon/mt.hpp>

int main() {
    std::cout << eon::mt::available() << std::endl;
    std::cout << eon::mt::concurrent_available() << std::endl;
    std::cout << eon::mt::optimal(5) << std::endl;
    std::cout << eon::mt::concurrent_optimal(5) << std::endl;

    return 0;
}
