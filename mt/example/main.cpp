#include <iostream>
#include <syncstream>

#include <eon/mt.hpp>

int main() {
    eon::mt::for_each(std::initializer_list{1, 2, 3, 4, 5}, [](int i) {
        std::osyncstream os(std::cout);
        os << i << std::endl;
    });

    std::cout << "\nSquares:\n";

    auto results = eon::mt::for_each_async(std::initializer_list{10, 15, 20}, [](int i) {
        return i * i;
    }, std::launch::async);

    for (auto & result : results) {
        std::cout << result.get() << std::endl;
    }

    return 0;
}
