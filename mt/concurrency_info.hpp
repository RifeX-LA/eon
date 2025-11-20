#pragma once

#include <thread>

namespace eon::mt {

    /**
     * @brief Returns the number of hardware thread contexts or 1 if this information is unavailable.
     */
    [[nodiscard]] inline unsigned int available() noexcept {
        return std::max(1u, std::thread::hardware_concurrency());
    }

    /**
     * @brief Returns the number of hardware thread contexts or 2 if this information is unavailable.
     */
    [[nodiscard]] inline unsigned int concurrent_available() noexcept {
        return std::max(2u, std::thread::hardware_concurrency());
    }

    /**
     * @brief Returns the optimal number of hardware thread contexts for task size of <b>n</b>.
     * Actually returns std::min(n, eon::mt::available()).
     */
    [[nodiscard]] inline unsigned int optimal(unsigned int const n) noexcept {
        return std::min(n, available());
    }

    /**
     * @brief Returns the optimal number of hardware thread contexts for a task size of <b>n</b>.
     * Actually returns std::min(n, eon::mt::concurrent_available()).
     */
    [[nodiscard]] inline unsigned int concurrent_optimal(unsigned int const n) noexcept {
        return std::min(n, concurrent_available());
    }

}
