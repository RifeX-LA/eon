#include <eon/lifetime.hpp>

std::size_t lifetime_total_from_peer() {
    return eon::lifetime::total();
}

std::size_t lifetime_alive_from_peer() {
    return eon::lifetime::alive();
}
