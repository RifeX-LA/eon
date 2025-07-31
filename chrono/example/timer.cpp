#include <iostream>
#include <vector>
#include <algorithm>

#include <eon/chrono.hpp>


[[nodiscard]] std::vector<int> create_vector(std::size_t const size) {
    return std::vector<int>(size);
}

void fill_vector(std::vector<int> & v) {
    std::ranges::generate(v, rand);
}

void sort_vector(std::vector<int> & v) {
    std::ranges::sort(v);
}

void push_back_to_vector(std::size_t const size) {
    std::wcout << "Push back to vector time: ";
    eon::chrono::wauto_timer<std::uintmax_t, std::milli> const auto_timer(std::wcout);

    std::vector<std::size_t> v;
    for (std::size_t i = 0; i < size; ++i) {
        v.push_back(i);
    }
}

int main() {
    eon::chrono::timer timer;
    std::vector<int> empty_vector;
    std::cout << "Create empty vector time: " << timer.elapsed() << "s\n";

    auto [elapsed_time, vector] = eon::chrono::timer<>::elapsed_r(create_vector, 100'000'000);
    std::cout << "Create vector with 100'000'000 elements time: " << elapsed_time << "s\n";

    auto const avg_creation_time = eon::chrono::timer<>::avg_elapsed(10, create_vector, 100'000'000);
    std::cout << "Create vector with 100'000'000 elements average time (10 iterations): " << avg_creation_time << "s\n";

    timer.reset();
    fill_vector(vector);
    std::cout << "Fill vector time: " << timer.elapsed() << "s\n";

    std::cout << "Sort vector time: " << eon::chrono::timer<std::uintmax_t, std::milli>::elapsed(sort_vector, vector) << "ms\n";

    push_back_to_vector(1'000'000);

    return 0;
}
