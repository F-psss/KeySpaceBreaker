#include <JSON_Protocol.hpp>
#include <iostream>

asio::awaitable<void> session(asio::ip::tcp::socket socket) {
    auto conn = JsonProtocol::Connection(std::move(socket));
    char data[1024];
    std::cout << "Клиент подключился" << std::endl;

    while (conn.is_open()) {
        JsonProtocol::Message request = co_await conn.read_message();
        std::cout << "Получен запрос: " << request.type << "/" << request.action
                  << std::endl;

        if (request.type == "quit" || request.action == "quit") {
            std::cout << "Клиент завершил работу" << std::endl;
            break;
        }

        std::cout << request.payload["fib"] << std::endl;

        // Выполнение какой-то нужной задачи
        int prev_fib = request.payload["prev_fib"];
        int fib = request.payload["fib"];
        int temp = fib;
        fib += prev_fib;
        prev_fib = temp;
        JsonProtocol::Message response = JsonProtocol::Message::create_response(
            request.action, {{"fib", fib}, {"prev_fib", prev_fib}}
        );

        std::cout << "Ожидание 1 секунду перед ответом..." << std::endl;
        asio::steady_timer timer(co_await asio::this_coro::executor);
        timer.expires_after(std::chrono::seconds(1));
        co_await timer.async_wait(asio::use_awaitable);

        co_await conn.send_message(response);
    }
}

asio::awaitable<void> listen(asio::ip::tcp::endpoint endpoint) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, endpoint);

    while (true) {
        asio::ip::tcp::socket socket =
            co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, session(std::move(socket)), asio::detached);
    }
}

int main() {
    asio::io_context io_context;
    asio::co_spawn(
        io_context, listen(asio::ip::tcp::endpoint{asio::ip::tcp::v4(), 12345}),
        asio::detached
    );

    io_context.run();
    return 0;
}
