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

asio::awaitable<void> WorkerSession::send_unit(std::size_t index) {
    m_current_unit_index = index;
    const Unit &unit = m_server.m_coordinator->get_unit(index);

    auto payload = std::make_unique<json_protocol::DecryptPayload>();
    payload->set_cipher(decrypt::CipherType::CAESAR);
   
    std::string text = m_server.m_coordinator->get_message()->get_text(
    );  
    payload->set_cipher_text(
        std::vector<uint8_t>(text.begin(), text.end())
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
            m_server.m_coordinator->mark_unit_done(m_current_unit_index.value());

            std::cout << "Received result from worker: key=" << cand_to_best.key_
                      << " score=" << cand_to_best.score_ << std::endl;

            if (m_server.m_coordinator->has_unassigned_units()) {
                asio::co_spawn(
                    m_conn.get_executor(),
                    m_server.m_coordinator->assign_to_worker(shared_from_this()),
                    asio::detached
                );
            } else if (m_server.m_coordinator->all_units_done()){
                std::cout << "All units are done. Sending final result to client..."
                          << std::endl;

                const auto &best = m_server.m_coordinator->best_result();

                std::cout << "\nBEST RESULT ON SERVER\n"
                          << "key   = " << best.key_ << "\n"
                          << "score = " << best.score_ << "\n"
                          << "text  = " << best.text_ << "\n";

                m_server.send_result_to_client(best);
            }

        }
    }
}

}  // namespace server
