#include <iostream>
#include <syncstream>

#include <eon/mt.hpp>

int main() {
    eon::mt::for_each(std::initializer_list{1, 2, 3, 4, 5}, [](int i) {
        std::osyncstream os(std::cout);
        os << i << std::endl;
    });

    return 0;
}
