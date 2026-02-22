#include "client.hpp"
#include <iostream>
#include <memory>
#include <string>
#include "../include/JSON_Protocol.hpp"

asio::awaitable<void> read_from_server(
    std::shared_ptr<json_protocol::Connection> conn
) {
    while (conn->is_open()) {
        json_protocol::Message response = co_await conn->read_message();

        // Используем фабричный метод для создания ответа
        if (response.get_action() == json_protocol::Action::PING) {
            // Получаем доступ к payload
            auto *payload = dynamic_cast<json_protocol::PingPayload *>(
                response.payload.get()
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
            auto request = json_protocol::Message::create_ping_request(
                std::move(new_payload)
            );

            std::cout << "Ожидание 1 секунду..." << std::endl;
            asio::steady_timer timer(co_await asio::this_coro::executor);
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait(asio::use_awaitable);

            co_await conn->send_message(request);
        }
    }
}

asio::awaitable<void> run_client(
    std::string host,
    int port,
    decrypt::CipherType cipher,
    std::vector<uint8_t> data
) {
    try {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::socket socket(executor);

        asio::ip::tcp::resolver resolver(executor);
        auto endpoints = co_await resolver.async_resolve(
            host, std::to_string(port), asio::use_awaitable
        );

        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

        auto conn =
            std::make_shared<json_protocol::Connection>(std::move(socket));

        // ===== формируем payload =====
        auto payload = std::make_unique<json_protocol::DecryptPayload>();

        payload->set_cipher(cipher);
        payload->set_cipher_text(data);
        payload->set_start_key(std::vector<uint8_t>{0});
        payload->set_end_key(std::vector<uint8_t>{255, 255});

        auto request =
            json_protocol::Message::create_decrypt_request(std::move(payload));

        // ===== отправляем =====
        co_await conn->send_message(request);

        // ===== ждём ответ =====
        auto response = co_await conn->read_message();

        std::cout << "Server responded:\n"
                  << response.to_json().dump(4) << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

// Основная корутина клиента
asio::awaitable<void> client() {
    try {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::socket socket(executor);

        std::cout << "Connecting..." << std::endl;

        co_await socket.async_connect(
            asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 12345),
            asio::use_awaitable
        );

        std::cout << "Connected! Type messages below." << std::endl;
        auto conn =
            std::make_shared<json_protocol::Connection>(std::move(socket));
        asio::co_spawn(executor, read_from_server(conn), asio::detached);

        while (socket.is_open()) {
            asio::steady_timer timer(executor);
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait(asio::use_awaitable);
        }

    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}
