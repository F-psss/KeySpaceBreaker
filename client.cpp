#include <asio.hpp>
#include <iostream>
#include <string>

using asio::ip::tcp;

asio::awaitable<void> read_from_server(tcp::socket &socket) {
    char buffer[1024];

    try {
        while (socket.is_open()) {
            std::size_t n = co_await socket.async_read_some(
                asio::buffer(buffer), asio::use_awaitable
            );

            std::string response(buffer, n);
            response.erase(
                std::remove(response.begin(), response.end(), '\n'),
                response.end()
            );

            std::cout << "Server: " << response;
            std::cout << "\n> " << std::flush;
        }
    } catch (const std::exception &e) {
        std::cout << "\nDisconnected: " << e.what() << std::endl;
    }
}

// Корутина для чтения с stdin
asio::awaitable<void> read_from_stdin(tcp::socket &socket) {
    auto executor = co_await asio::this_coro::executor;
    asio::posix::stream_descriptor stdin_stream(executor, ::dup(STDIN_FILENO));

    char buffer[1024];

    try {
        while (socket.is_open()) {
            // Асинхронно читаем из stdin
            std::size_t n = co_await stdin_stream.async_read_some(
                asio::buffer(buffer), asio::use_awaitable
            );

            std::string input(buffer, n);

            input.erase(
                std::remove(input.begin(), input.end(), '\n'), input.end()
            );

            if (input == "quit") {
                std::cout << "Exiting..." << std::endl;
                socket.close();
                break;
            }

            if (!input.empty()) {
                input += "\n";
                co_await asio::async_write(
                    socket, asio::buffer(input), asio::use_awaitable
                );
            }
        }
    } catch (const std::exception &e) {
        std::cout << "Input error: " << e.what() << std::endl;
    }
}

// Основная корутина клиента
asio::awaitable<void> client() {
    try {
        auto executor = co_await asio::this_coro::executor;
        tcp::socket socket(executor);

        std::cout << "Connecting..." << std::endl;

        co_await socket.async_connect(
            tcp::endpoint(asio::ip::make_address("127.0.0.1"), 12345),
            asio::use_awaitable
        );

        std::cout << "Connected! Type messages below." << std::endl;

        asio::co_spawn(executor, read_from_server(socket), asio::detached);
        asio::co_spawn(executor, read_from_stdin(socket), asio::detached);

        while (socket.is_open()) {
            asio::steady_timer timer(executor);
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait(asio::use_awaitable);
        }

    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

int main() {
    asio::io_context io_context;
    asio::co_spawn(io_context, client(), asio::detached);
    io_context.run();

    return 0;
}
