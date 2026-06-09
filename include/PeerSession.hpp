#pragma once

#include <asio.hpp>
#include <memory>
#include <optional>
#include "JSON_Protocol.hpp"

namespace server {

class CoordinatorServer;

class PeerSession : public std::enable_shared_from_this<PeerSession> {
public:
    PeerSession(
        asio::ip::tcp::socket socket,
        CoordinatorServer &server,
        bool is_initiator
    );

    void start();

    asio::awaitable<void>
    send_hello(int my_id, int my_role, json_protocol::MessageType type);

    std::optional<int> peer_id() const {
        return m_peer_id;
    }

    int peer_role() const {
        return m_peer_role;
    }

    bool is_initiator() const {
        return m_is_initiator;
    }

    bool is_open() const {
        return m_conn.is_open();
    }

    void close() {
        m_conn.close();
    }

    void close_intentionally() {
        m_closing_intentionally = true;
        m_conn.close();
    }

    asio::awaitable<void> send_raw(const json_protocol::Message &msg);

    void set_peer_role(int role) {
        m_peer_role = role;
    }

private:
    json_protocol::Connection m_conn;
    CoordinatorServer &m_server;
    std::optional<int> m_peer_id;
    int m_peer_role = 1;  // 0 = Primary, 1 = Backup
    bool m_is_initiator;
    bool m_closing_intentionally = false;

    asio::awaitable<void> read_loop();
    void handle_message(const json_protocol::Message &msg);
    void handle_hello(const json_protocol::Message &msg);
};

}  // namespace server