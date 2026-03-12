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

}  // namespace server
