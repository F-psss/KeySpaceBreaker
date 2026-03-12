#include <CoordServ.hpp>
#include <asio/co_spawn.hpp>
#include <iostream>
#include "Unit.hpp"

namespace server {

// ---------- WorkerSession ----------

WorkerSession::WorkerSession(
    asio::ip::tcp::socket socket,
    CoordinatorServer &server
)
    : m_conn(std::move(socket)), m_server(server) {
}

void WorkerSession::start() {
    asio::co_spawn(m_conn.get_executor(), read_loop(), asio::detached);
}

asio::awaitable<void> WorkerSession::send_unit(const Unit &unit) {
    auto payload = std::make_unique<json_protocol::DecryptPayload>();
    payload->set_cipher(decrypt::CipherType::CAESAR);
    // В реальном коде нужно передать зашифрованный текст и диапазон ключей.
    // Здесь используем упрощение: передаём только диапазон, текст должен быть
    // известен воркеру иначе. Для полноты нужно либо хранить текст в
    // Coordinator и передавать вместе с юнитом, либо использовать другой
    // подход. Пока заглушка.
    std::string dummy_text = m_server.m_coordinator->get_message()->get_text(
    );  // замените на реальный текст из m_message
    payload->set_cipher_text(
        std::vector<uint8_t>(dummy_text.begin(), dummy_text.end())
    );
    payload->set_start_key(std::vector<uint8_t>{
        static_cast<uint8_t>(unit.get_start())});
    payload->set_end_key(std::vector<uint8_t>{
        static_cast<uint8_t>(unit.get_end())});

    auto msg =
        json_protocol::Message::create_decrypt_request(std::move(payload));
    co_await m_conn.send_message(msg);
}

asio::awaitable<void> WorkerSession::read_loop() {
    try {
        while (m_conn.is_open()) {
            auto msg = co_await m_conn.read_message();
            handle_message(msg);
        }
    } catch (const std::exception &e) {
        std::cerr << "Worker session error: " << e.what() << std::endl;
    }
}

void WorkerSession::handle_message(const json_protocol::Message &msg) {
    if (msg.get_type() == json_protocol::MessageType::REQUEST &&
        msg.get_action() == json_protocol::Action::STATUS) {
        auto *payload =
            dynamic_cast<json_protocol::StatusPayload *>(msg.payload.get());
        if (payload != nullptr) {
            Result cand_to_best;
            cand_to_best.key_ = payload->get_key();
            cand_to_best.text_ = payload->get_cipher_text();
            cand_to_best.score_ = payload->get_score();
            m_server.m_coordinator->cand_to_best(cand_to_best);

            asio::co_spawn(
                m_conn.get_executor(),
                m_server.m_coordinator->assign_to_worker(shared_from_this()),
                asio::detached
            );
            std::cout << "Received result from worker\n";
        }
    }
}

}  // namespace server
