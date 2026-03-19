#pragma once

#include <asio.hpp>
#include <memory>
#include <vector>
#include "CaesarEncryptedMessage.hpp"
#include "JSON_Protocol.hpp"
#include "StaticPolicy.hpp"

namespace server {
class CoordinatorServer;

// Сессия клиента (один)
class ClientSession {
public:
    ClientSession(asio::ip::tcp::socket socket, CoordinatorServer &server);
    void start();
    asio::awaitable<void> send_final_result(const Result &result);

private:
    json_protocol::Connection m_conn;
    CoordinatorServer &m_server;

    asio::awaitable<void> read_loop();
    void handle_task_request(const json_protocol::Message &msg);
};

}  // namespace server
