#include <iostream>
#include "../include/JSON_Protocol.hpp"

asio::awaitable<void> session(asio::ip::tcp::socket socket) {
    auto conn = json_protocol::Connection(std::move(socket));
    char data[1024];
    std::cout << "Клиент подключился" << std::endl;

    while (conn.is_open()) {
        json_protocol::Message request = co_await conn.read_message();
        std::cout << "Получен запрос: "
                  << json_protocol::type_to_string(request.get_type()) << "/"
                  << json_protocol::action_to_string(request.get_action())
                  << std::endl;

        if (request.get_type() == json_protocol::MessageType::ERROR ||
            request.get_action() == json_protocol::Action::QUIT) {
            std::cout << "Клиент завершил работу" << std::endl;
            break;
        }

        // std::cout << request.payload["fib"] << std::endl;

        // Выполнение какой-то нужной задачи

        // Используем фабричный метод для создания ответа
        if (request.get_action() == json_protocol::Action::DECRYPT) {
            // Получаем доступ к payload
            auto *payload = dynamic_cast<json_protocol::DecryptPayload *>(
                request.payload.get()
            );

            if (!payload) {
                std::cerr << "Invalid decrypt payload\n";
                continue;
            }

            std::cout << "Decrypt request received\n";
            std::cout << "Cipher: "
                      << json_protocol::cipher_to_string(payload->get_cipher())
                      << "\n";

            std::cout << "Text: "
                      << std::string(
                             payload->get_cipher_text().begin(),
                             payload->get_cipher_text().end()
                         )
                      << "\n";

            std::cout << "Range: "
                      << std::string(
                             payload->get_start_key().begin(),
                             payload->get_start_key().end()
                         )
                      << " - "
                      << std::string(
                             payload->get_end_key().begin(),
                             payload->get_end_key().end()
                         )
                      << "\n";
        }

        // auto new_payload = std::make_unique<json_protocol::PingPayload>();
        // new_payload->set_code(next_fib);
        // auto response = json_protocol::Message::create_pong_response(
        //     std::move(new_payload)
        // );

        // std::cout << "Ожидание 1 секунду перед ответом..." << std::endl;
        // asio::steady_timer timer(co_await asio::this_coro::executor);
        // timer.expires_after(std::chrono::seconds(1));
        // co_await timer.async_wait(asio::use_awaitable);
        //
        // co_await conn.send_message(response);
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
