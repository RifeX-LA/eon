#include <iostream>

#include <eon/tpl.hpp>

template <typename Tuple>
struct widget {
    Tuple & tuple;

    widget(widget const &) = delete;
};

int main() {
    std::tuple tuple{1, "Hello", 2.5};
    std::cout << eon::tpl::io(tuple);

    return 0;
}
