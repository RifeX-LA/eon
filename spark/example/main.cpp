#include <vector>
#include <optional>
#include <print>

#include <eon/spark.hpp>

int main() {
    std::vector<std::optional<int>> v {1, std::nullopt, 42};

    std::println("{}", v | eon::views::has_value | eon::views::indirect);

    return 0;
}
