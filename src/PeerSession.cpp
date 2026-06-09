#include <CoordServ.hpp>
#include <PeerSession.hpp>
#include <asio/co_spawn.hpp>
#include <iostream>

namespace server {

PeerSession::PeerSession(
    asio::ip::tcp::socket socket,
    CoordinatorServer &server,
    bool is_initiator
)
    : m_conn(std::move(socket)),
      m_server(server),
      m_is_initiator(is_initiator) {
}

void PeerSession::start() {
    asio::co_spawn(m_conn.get_executor(), read_loop(), asio::detached);
}

asio::awaitable<void> PeerSession::send_hello(
    int my_id,
    int my_role,
    json_protocol::MessageType type
) {
    if (m_closing_intentionally) {
        co_return;
    }
    try {
        auto payload =
            std::make_unique<json_protocol::HelloPayload>(my_id, my_role);
        auto msg = (type == json_protocol::MessageType::REQUEST)
                       ? json_protocol::Message::create_peer_hello_request(
                             std::move(payload)
                         )
                       : json_protocol::Message::create_peer_hello_response(
                             std::move(payload)
                         );
        co_await m_conn.send_message(msg);
    } catch (const std::exception &e) {
        std::cerr << "PeerSession::send_hello failed: " << e.what()
                  << std::endl;
    }
}

asio::awaitable<void> PeerSession::read_loop() {
    try {
        while (m_conn.is_open()) {
            auto msg = co_await m_conn.read_message();
            handle_message(msg);
        }
    } catch (const std::exception &e) {
        if (m_closing_intentionally) {
            std::cout << "[Peer "
                      << (m_peer_id ? std::to_string(*m_peer_id) : "?")
                      << "] connection closed (dedup)" << std::endl;
        } else {
            std::cerr << "[Peer "
                      << (m_peer_id ? std::to_string(*m_peer_id) : "?")
                      << "] connection lost: " << e.what() << std::endl;
            m_server.on_peer_disconnected(shared_from_this());
        }
    }
}

void PeerSession::handle_message(const json_protocol::Message &msg) {
    auto action = msg.get_action();

    if (action == json_protocol::Action::PEER_HELLO) {
        handle_hello(msg);
        return;
    }
    if (action == json_protocol::Action::PEER_PING) {
        m_server.on_primary_ping();
        return;
    }
    if (action == json_protocol::Action::PEER_ELECTION) {
        auto *p =
            dynamic_cast<json_protocol::PeerIdPayload *>(msg.payload.get());
        if (p) {
            m_server.on_peer_election(p->get_peer_id());
        }
        return;
    }
    if (action == json_protocol::Action::PEER_ALIVE) {
        auto *p =
            dynamic_cast<json_protocol::PeerIdPayload *>(msg.payload.get());
        if (p) {
            m_server.on_peer_alive(p->get_peer_id());
        }
        return;
    }
    if (action == json_protocol::Action::PEER_COORDINATOR) {
        auto *p =
            dynamic_cast<json_protocol::PeerIdPayload *>(msg.payload.get());
        if (p) {
            m_server.on_peer_coordinator(p->get_peer_id());
        }
        return;
    }

    std::cout << "[Peer "
              << (m_peer_id ? std::to_string(*m_peer_id) : "unknown")
              << "] received unhandled action" << std::endl;
}

void PeerSession::handle_hello(const json_protocol::Message &msg) {
    auto *payload =
        dynamic_cast<json_protocol::HelloPayload *>(msg.payload.get());
    if (payload == nullptr) {
        std::cerr << "PEER_HELLO without HelloPayload" << std::endl;
        return;
    }

    m_peer_id = payload->get_peer_id();
    m_peer_role = payload->get_role();
    std::cout << "[Peer] HELLO from id=" << *m_peer_id
              << " role=" << (m_peer_role == 0 ? "Primary" : "Backup")
              << std::endl;

    if (msg.get_type() == json_protocol::MessageType::REQUEST) {
        auto self = shared_from_this();
        int my_id = m_server.get_id();
        int my_role = (m_server.get_role() == Role::Primary) ? 0 : 1;
        asio::co_spawn(
            m_conn.get_executor(),
            [self, my_id, my_role]() -> asio::awaitable<void> {
                co_await self->send_hello(
                    my_id, my_role, json_protocol::MessageType::RESPONSE
                );
            },
            asio::detached
        );
    }
    m_server.on_peer_hello(shared_from_this());
}

asio::awaitable<void> PeerSession::send_raw(const json_protocol::Message &msg) {
    if (m_closing_intentionally) {
        co_return;
    }
    try {
        co_await m_conn.send_message(msg);
    } catch (const std::exception &e) {
        std::cerr << "PeerSession::send_raw failed: " << e.what() << std::endl;
    }
}

}  // namespace server