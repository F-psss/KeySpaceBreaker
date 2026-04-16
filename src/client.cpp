#include "client.hpp"
#include <iostream>
#include <memory>
#include <string>
#include "JSON_Protocol.hpp"

asio::awaitable<void> run_client(
const app_config::ClientConfig& cfg
) {
    try {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::socket socket(executor);

        asio::ip::tcp::resolver resolver(executor);
        auto endpoints = co_await resolver.async_resolve(
    cfg.coordinator_host,
    std::to_string(cfg.coordinator_port),
    asio::use_awaitable
);

        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

        auto conn =
            std::make_shared<json_protocol::Connection>(std::move(socket));

        // ===== формируем payload =====
        auto payload = std::make_unique<json_protocol::DecryptPayload>();

        payload->set_cipher(cfg.cipher);
        payload->set_noise(cfg.noise);
        payload->set_cipher_text(cfg.encrypted_data);
        payload->set_start_key(std::vector<uint8_t>{0});
        payload->set_end_key(std::vector<uint8_t>{24});

        auto request =
            json_protocol::Message::create_decrypt_request(std::move(payload));

        // ===== отправляем =====
        co_await conn->send_message(request);

        // ===== ждём ответ =====
        auto response = co_await conn->read_message();

        if (response.get_type() == json_protocol::MessageType::RESPONSE &&
            response.get_action() == json_protocol::Action::STATUS) {
            auto *payload =
                dynamic_cast<json_protocol::StatusPayload *>(response.payload.get());

            if (payload != nullptr) {
                std::cout << "\n===== BEST RESULT =====\n";
                std::cout << "Key:   " << payload->get_key() << "\n";
                std::cout << "Score: " << payload->get_score() << "\n";
                std::cout << "Text:  " << payload->get_cipher_text() << "\n";
                std::cout << "=======================\n";
            } else {
                std::cout << "Server responded:\n"
                          << response.to_json().dump(4) << std::endl;
            }
        } else {
            std::cout << "Server responded:\n"
                      << response.to_json().dump(4) << std::endl;
        }

    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}
