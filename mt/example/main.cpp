#include <iostream>
#include <syncstream>

#include <eon/mt.hpp>

int main() {
    eon::mt::thread_pool<int> thread_pool;

    auto future1 = thread_pool.add_task([] {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 42;
    });

    auto future2 = thread_pool.add_task([] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 12;
    });

    std::cout << future1.get() << ' ' << future2.get() << std::endl;

    return 0;
}
