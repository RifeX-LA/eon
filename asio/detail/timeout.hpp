#pragma once

#include <algorithm>
#include <boost/asio.hpp>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace eon::asio::detail {

struct timeout_state {
    boost::asio::ip::tcp::socket * socket;
    std::chrono::steady_clock::time_point deadline;
};

inline thread_local timeout_state * active_timeout = nullptr;

inline void check_deadline(timeout_state const & state) {
    if (std::chrono::steady_clock::now() >= state.deadline) {
        throw boost::system::system_error(boost::asio::error::timed_out);
    }
}

class timeout_scope {
    boost::asio::ip::tcp::socket & socket_;
    bool previous_mode_;
    timeout_state state_;
    timeout_state * previous_;

public:
    template <typename Rep, class Period>
    timeout_scope(boost::asio::ip::tcp::socket & socket, std::chrono::duration<Rep, Period> timeout)
        : socket_(socket), previous_mode_(socket.non_blocking()), state_{&socket, {}}, previous_(active_timeout) {
        using clock = std::chrono::steady_clock;

        auto const seconds = std::chrono::duration<long double>(timeout).count();
        if (!std::isfinite(seconds)) {
            throw std::invalid_argument("timeout must be finite");
        }

        auto const now = clock::now();
        auto const maximum = clock::time_point::max() - now;
        state_.deadline = seconds >= std::chrono::duration<long double>(maximum).count()
                              ? clock::time_point::max()
                              : now + std::chrono::duration_cast<clock::duration>(
                                          std::chrono::duration<long double>(std::max(0.L, seconds)));

        if (previous_ && previous_->socket == &socket) {
            state_.deadline = std::min(state_.deadline, previous_->deadline);
        }
        check_deadline(state_);
        socket_.non_blocking(true);
        active_timeout = &state_;
    }

    timeout_scope(timeout_scope const &) = delete;
    timeout_scope & operator=(timeout_scope const &) = delete;

    ~timeout_scope() {
        active_timeout = previous_;
        boost::system::error_code ignored;
        socket_.non_blocking(previous_mode_, ignored);
    }
};

template <bool Write, class Buffer>
void transfer_bytes(boost::asio::ip::tcp::socket & socket, Buffer buffer) {
    auto const state = active_timeout;
    if (!state || state->socket != &socket) {
        if constexpr (Write) {
            boost::asio::write(socket, buffer);
        } else {
            boost::asio::read(socket, buffer);
        }
        return;
    }
    while (buffer.size() != 0) {
        check_deadline(*state);
        boost::system::error_code ec;
        std::size_t count;
        if constexpr (Write) {
            count = socket.write_some(buffer, ec);
        } else {
            count = socket.read_some(buffer, ec);
        }
        buffer += count;
        if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
            std::this_thread::sleep_until(
                std::min(state->deadline, std::chrono::steady_clock::now() + std::chrono::milliseconds(1)));
        } else if (ec) {
            throw boost::system::system_error(ec);
        } else if (count == 0) {
            throw boost::system::system_error(boost::asio::error::eof);
        }
    }
    check_deadline(*state);
}

inline void write_bytes(boost::asio::ip::tcp::socket & socket, boost::asio::const_buffer buffer) {
    transfer_bytes<true>(socket, buffer);
}

inline void read_bytes(boost::asio::ip::tcp::socket & socket, boost::asio::mutable_buffer buffer) {
    transfer_bytes<false>(socket, buffer);
}

} // namespace eon::asio::detail
