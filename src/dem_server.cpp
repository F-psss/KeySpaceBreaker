#include <JSON_Protocol.hpp>
#include <iostream>

asio::awaitable<void> session(asio::ip::tcp::socket socket) {
    auto conn = json_protocol::Connection(std::move(socket));
    char data[1024];
    std::cout << "Клиент подключился" << std::endl;

    while (conn.is_open()) {
        json_protocol::Message request = co_await conn.read_message();
        std::cout << "Получен запрос: \n";
        std::cout << "Получен запрос: "
                  << json_protocol::type_to_string(request.get_type()) << "/"
                  << json_protocol::action_to_string(request.get_action())
                  << std::endl;

        json_protocol::Message response;

        if (request.get_type() == json_protocol::MessageType::ERROR ||
            request.get_action() == json_protocol::Action::QUIT) {
            std::cout << "Клиент завершил работу" << std::endl;
            break;
        }

        // std::cout << request.payload["fib"] << std::endl;

        // Выполнение какой-то нужной задачи

        // Используем фабричный метод для создания ответа
        if (request.get_action() == json_protocol::Action::PING) {
            // Получаем доступ к payload
            auto *payload =
                dynamic_cast<json_protocol::PingPayload *>(request.payload.get()
                );
            if (payload == nullptr) {
                std::cerr << "Invalid payload for FIBONACCI action"
                          << std::endl;
                continue;
            }

            std::cout << "fib: " << payload->get_code() << std::endl;

            // Вычисляем следующее число
            int next_fib = payload->get_code() + 1;

            auto new_payload = std::make_unique<json_protocol::PingPayload>();
            new_payload->set_code(next_fib);
            response = json_protocol::Message::create_pong_response(
                std::move(new_payload)
            );
        }

        if (request.get_action() == json_protocol::Action::DECRYPT) {
            auto *payload = dynamic_cast<json_protocol::DecryptPayload *>(
                request.payload.get()
            );
            if (payload == nullptr) {
                std::cerr << "Invalid payload for FIBONACCI action"
                          << std::endl;
                continue;
            }

            std::cout << "new: "
                      << payload->get_cipher_text()[payload->get_start_key()[0]]
                      << std::endl;

            auto new_payload =
                std::make_unique<json_protocol::DecryptPayload>();
            new_payload->set_cipher(decrypt::CipherType::XOR);
            new_payload->set_cipher_text(payload->get_cipher_text());

            uint8_t start_key_value = payload->get_start_key()[0] + 1;
            new_payload->set_start_key(std::vector<uint8_t>{start_key_value});

            new_payload->set_end_key(std::vector<uint8_t>{255});
            response = json_protocol::Message::create_decrypt_response(
                std::move(new_payload)
            );
        }
        std::cout << "Ожидание 1 секунду перед ответом..." << std::endl;
        asio::steady_timer timer(co_await asio::this_coro::executor);
        timer.expires_after(std::chrono::seconds(1));
        co_await timer.async_wait(asio::use_awaitable);

        co_await conn.send_message(response);
    }
    std::cout << "Получен запрос: ";
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
