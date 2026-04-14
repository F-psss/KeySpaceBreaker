#pragma once

#include <asio.hpp>
#include <memory>
#include <vector>
#include "EncryptedMessage.hpp"
#include "JSON_Protocol.hpp"
#include "Policy.hpp"

namespace server {
class CoordinatorServer;

// Сессия воркера
class WorkerSession : public std::enable_shared_from_this<WorkerSession> {
public:
    WorkerSession(asio::ip::tcp::socket socket, CoordinatorServer &server);
    void start();


    // Отправить юнит воркеру
    asio::awaitable<void> send_unit(std::size_t index);

private:
    json_protocol::Connection m_conn;
    CoordinatorServer &m_server;
    asio::awaitable<void> read_loop();
    std::optional<std::size_t> m_current_unit_index;
    void handle_message(const json_protocol::Message &msg);
};

}  // namespace server
