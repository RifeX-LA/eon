#include <iostream>

#include <eon/asio.hpp>

int main() {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    auto const value = eon::asio::read_as_buffer<int>(socket);

    return 0;
}