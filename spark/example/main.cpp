#include <iostream>
#include <optional>
#include <print>
#include <vector>

#include <eon/spark.hpp>

void foo(auto doing_smth) {
    int a = 10;
    doing_smth(a);
}

int main() {
    foo(eon::spark::stub);

    std::vector<std::pair<int, int>> const v{{1, 2}, {2, 0}, {5, 6}};
    for (auto const [key, value] : v | std::views::filter(eon::spark::always(true))) {
        std::cout << key << ": " << value << "\n";
    }

    std::vector<int> v2;
    std::ranges::transform(v | std::views::keys | std::views::filter(eon::spark::even), std::back_inserter(v2),
                           eon::spark::remainder(3));

    std::ranges::for_each(v2, eon::spark::assign(eon::spark::add(2)));

    return 0;
}
