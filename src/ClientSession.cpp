#include <ClientSession.hpp>
#include <iostream>

namespace server {

// ---------- ClientSession ----------

ClientSession::ClientSession(
    asio::ip::tcp::socket socket,
    CoordinatorServer &server
)
    : m_conn(std::move(socket)), m_server(server) {
}

void ClientSession::start() {
    asio::co_spawn(m_conn.get_executor(), read_loop(), asio::detached);
}

asio::awaitable<void> ClientSession::read_loop() {
    try {
        auto msg =
            co_await m_conn.read_message();  // ожидаем одно сообщение с задачей
        handle_task_request(msg);
    } catch (const std::exception &e) {
        std::cerr << "Client session error: " << e.what() << std::endl;
    }
}

asio::awaitable<void> ClientSession::send_final_result(const Result &result) {
    auto payload = std::make_unique<json_protocol::StatusPayload>(
        decrypt::CipherType::CAESAR,
        result.text_,
        result.key_,
        result.score_
    );

    auto msg =
        json_protocol::Message::create_status_response(std::move(payload));

    co_await m_conn.send_message(msg);
}

}  // namespace server
