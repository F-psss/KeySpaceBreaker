#include <NetworkWorker.hpp>
#include <asio/co_spawn.hpp>
#include <memory>
#include "CaesarEncryptedMessage.hpp"
#include "Decryptor.hpp"
#include "EncryptedMessage.hpp"
#include "JSON_Protocol.hpp"

void Worker::start() {
    asio::co_spawn(m_io, connect(), asio::detached);
}

void Worker::stop() {
    if (m_conn && m_conn->is_open()) {
        m_conn->close();
    }
}

asio::awaitable<void> Worker::connect() {
    while (true) {
        try {
            asio::ip::tcp::endpoint endpoint(
                asio::ip::make_address(m_coordinator_ip), m_coordinator_port
            );
            asio::ip::tcp::socket socket(m_io);
            co_await socket.async_connect(endpoint, asio::use_awaitable);

            m_conn =
                std::make_shared<json_protocol::Connection>(std::move(socket));
            std::cout << "Worker connected to coordinator" << std::endl;

            co_await run();
        } catch (const std::exception &e) {
            std::cerr << "Connection error: " << e.what() << std::endl;
        }
        asio::steady_timer timer(m_io);
        timer.expires_after(std::chrono::seconds(5));
        co_await timer.async_wait(asio::use_awaitable);
    }
}

asio::awaitable<void> Worker::run() {
    try {
        while (m_conn && m_conn->is_open()) {
            auto msg = co_await m_conn->read_message();
            handle_message(msg);
        }
    } catch (...) {
    }
}

void Worker::handle_message(const json_protocol::Message &msg) {
    std::cout << "Получен запрос: "
              << json_protocol::type_to_string(msg.get_type()) << "/"
              << json_protocol::action_to_string(msg.get_action()) << std::endl;

    if (msg.get_type() == json_protocol::MessageType::REQUEST &&
        msg.get_action() == json_protocol::Action::DECRYPT) {
        auto *payload =
            dynamic_cast<json_protocol::DecryptPayload *>(msg.payload.get());
        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            auto enc =
                std::make_shared<server::CaesarEncryptedMessage>(std::string(
                    payload->get_cipher_text().begin(),
                    payload->get_cipher_text().end()
                ));
            auto caesar_worker = std::make_unique<server::CaesarDecryptor>(enc);
            m_decryptor = std::move(caesar_worker);

            auto unit = std::make_shared<server::Unit>(
                static_cast<int>(payload->get_start_key()[0]),
                static_cast<int>(payload->get_end_key()[0]),
                    payload->get_noise()
            );
            asio::co_spawn(
                m_conn->get_executor(), run_decryptor_task(unit), asio::detached
            );

        } else {
            std::cout << "WRONG TYPE OF CHIPHER\n\n\n";
        }
    }
}

asio::awaitable<void> Worker::run_decryptor_task(
    std::shared_ptr<server::Unit> unit
) {
    try {
        std::cout << "РЕЗУЛЬТАТ: "
                  << "\n";
        auto *caesar =
            dynamic_cast<server::CaesarDecryptor *>(m_decryptor.get());
        if (caesar == nullptr) {
            throw std::runtime_error("Wrong decryptor type");
        }

        caesar->process_unit(unit);  // передаём unit напрямую
        auto result = caesar->get_best_result();
        std::cout << "РЕЗУЛЬТАТ: " << result.text_ << ' ' << result.score_
                  << "\n";

        auto st_payload = std::make_unique<json_protocol::StatusPayload>(
            decrypt::CipherType::CAESAR, result.text_, result.key_,
            result.score_
        );
        auto res_msg =
            json_protocol::Message::create_status_request(std::move(st_payload)
            );
        co_await m_conn->send_message(res_msg);
    } catch (const std::exception &e) {
        std::cerr << "Exception in run_decryptor_task: " << e.what()
                  << std::endl;
    }
}
