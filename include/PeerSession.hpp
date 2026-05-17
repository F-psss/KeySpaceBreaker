#pragma once

#include <asio.hpp>
#include <memory>
#include <optional>
#include "JSON_Protocol.hpp"

namespace server {

class CoordinatorServer;

class PeerSession : public std::enable_shared_from_this<PeerSession> {
public:
    
    PeerSession(asio::ip::tcp::socket socket, CoordinatorServer &server);

   
    void start();


    asio::awaitable<void> send_hello(int my_id, json_protocol::MessageType type);


    std::optional<int> peer_id() const {
        return m_peer_id;
    }

    bool is_open() const {
        return m_conn.is_open();
    }

private:
    json_protocol::Connection m_conn;
    CoordinatorServer &m_server;
    std::optional<int> m_peer_id;

    asio::awaitable<void> read_loop();
    void handle_message(const json_protocol::Message &msg);
    void handle_hello(const json_protocol::Message &msg);
};

}  // namespace server