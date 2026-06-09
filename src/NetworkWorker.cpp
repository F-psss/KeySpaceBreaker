#include <NetworkWorker.hpp>
#include <asio/co_spawn.hpp>
#include <config.hpp>
#include <memory>
#include "CaesarEncryptedMessage.hpp"
#include "Decryptor.hpp"
#include "EncryptedMessage.hpp"
#include "JSON_Protocol.hpp"
#include "VigenereEncryptedMessage.hpp"

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
        bool connected_this_round = false;

        for (const auto &address : m_coordinator_addresses) {
            auto pos = address.find(':');
            if (pos == std::string::npos) {
                std::cerr << "Invalid coordinator address: " << address << std::endl;
                continue;
            }
            std::string host = address.substr(0, pos);
            std::string port_str = address.substr(pos + 1);

            try {
                asio::ip::tcp::socket socket(m_io);
                asio::ip::tcp::resolver resolver(m_io);
                auto endpoints = co_await resolver.async_resolve(
                    host, port_str, asio::use_awaitable);
                co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

                m_conn = std::make_shared<json_protocol::Connection>(std::move(socket));
                std::cout << "Worker connected to coordinator " << address << std::endl;

                connected_this_round = true;
                co_await run();

                std::cout << "Connection to " << address << " lost, will retry" << std::endl;
                m_conn.reset();
                break;

            } catch (const std::exception &e) {
                std::cerr << "Cannot connect to " << address << ": "
                          << e.what() << " — trying next" << std::endl;
                continue;
            }
        }

        if (!connected_this_round) {
            asio::steady_timer timer(m_io);
            timer.expires_after(std::chrono::seconds(2));
            co_await timer.async_wait(asio::use_awaitable);
        }
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
    // std::cout << "Получен запрос: "
    //           << json_protocol::type_to_string(msg.get_type()) << "/"
    //           << json_protocol::action_to_string(msg.get_action()) <<
    //           std::endl;

    if (msg.get_type() == json_protocol::MessageType::REQUEST &&
        msg.get_action() == json_protocol::Action::DECRYPT) {
        auto *payload =
            dynamic_cast<json_protocol::DecryptPayload *>(msg.payload.get());
        // Кэширование: если в payload есть cipher_text, сохраняем
        if (!payload->get_cipher_text().empty()) {
            m_cachedCiphertext = payload->get_cipher_text();
            m_hasCiphertext = true;
        }

        // Используем кэшированный текст для дешифрования
        if (!m_hasCiphertext) {
            std::cerr << "Error: No ciphertext received yet" << std::endl;
            return;
        }

        // Далее используем m_cachedCiphertext вместо payload->get_cipher_text()
        const std::string text(m_cachedCiphertext.begin(), m_cachedCiphertext.end());
        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            auto enc = std::make_shared<server::CaesarEncryptedMessage>(text);
            auto caesar_worker =
                std::make_unique<server::PolyAlphabeticDecryptor>(enc, m_dict_path);
            m_decryptor = std::move(caesar_worker);
            auto start_vec = payload->get_start_key();
            auto end_vec = payload->get_end_key();
            if (start_vec.empty() || end_vec.empty()) {
                std::cerr << "Error: start_key or end_key is empty\n";
                return;
            }
            auto unit = std::make_shared<server::Unit>(
                static_cast<int>(payload->get_start_key()[0]),
                static_cast<int>(payload->get_end_key()[0]),
                payload->get_cipher(), payload->get_mode(), 1,
                payload->get_noise()
            );
            asio::co_spawn(
                m_conn->get_executor(), run_decryptor_task(unit), asio::detached
            );

        } else if (payload->get_cipher() == decrypt::CipherType::VIGENERE) {
            auto enc =
                std::make_shared<server::VigenereEncryptedMessage>(text);
            std::string start_key_str(
                payload->get_start_key().begin(), payload->get_start_key().end()
            );
            std::string end_key_str(
                payload->get_end_key().begin(), payload->get_end_key().end()
            );
            auto key_to_index = [](const std::string &key) -> int {
                int idx = 0;
                for (char c : key) {
                    idx = idx * 26 + (c - 'A');
                }
                return idx;
            };

            int start = key_to_index(start_key_str);
            int end = key_to_index(end_key_str);

            auto vigenere_worker =
                std::make_unique<server::PolyAlphabeticDecryptor>(enc, m_dict_path);
            m_decryptor = std::move(vigenere_worker);

            auto unit = std::make_shared<server::Unit>(
                start, end, payload->get_cipher(), payload->get_mode(),
                payload->get_key_length(), payload->get_noise()
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
        if (!m_decryptor) {
            throw std::runtime_error("No decryptor");
        }

        m_decryptor->process_unit(unit);
        auto result = m_decryptor->get_best_result();

        // std::cout << "РЕЗУЛЬТАТ: " << result.text_ << ' ' << result.score_
        //           << "\n";

        auto st_payload = std::make_unique<json_protocol::StatusPayload>(
            unit->get_cipher(), result.text_, result.key_, result.score_
        );
        auto res_msg = json_protocol::Message::create_status_request(
            std::move(st_payload)
        );
        co_await m_conn->send_message(res_msg);
    } catch (const std::exception &e) {
        std::cerr << "Exception in run_decryptor_task: " << e.what()
                  << std::endl;
    }
}
