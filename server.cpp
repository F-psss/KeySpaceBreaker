#include <asio.hpp>
#include <iostream>

using asio::ip::tcp;

asio::awaitable<void> session(tcp::socket socket) {
    char data[1024];

    while (true) {
        std::size_t n = co_await socket.async_read_some(
            asio::buffer(data), asio::use_awaitable
        );

        std::string message(data, n);
        std::cout << message;
        message.erase(
            std::remove(message.begin(), message.end(), '\n'), message.end()
        );
        message.erase(
            std::remove(message.begin(), message.end(), '\r'), message.end()
        );

        if (message == "Hi!") {
            co_await async_write(
                socket, asio::buffer("HI!", 3), asio::use_awaitable
            );
        } else {
            co_await async_write(
                socket, asio::buffer(data, n), asio::use_awaitable
            );
        }
    }
}

asio::awaitable<void> listen(tcp::endpoint endpoint) {
    auto executor = co_await asio::this_coro::executor;
    tcp::acceptor acceptor(executor, endpoint);

    while (true) {
        tcp::socket socket =
            co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, session(std::move(socket)), asio::detached);
    }
}

int main() {
    asio::io_context io_context;
    asio::co_spawn(
        io_context, listen(tcp::endpoint{tcp::v4(), 12345}), asio::detached
    );

    io_context.run();
    return 0;
}
