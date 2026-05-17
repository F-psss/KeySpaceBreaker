#include <PeerSession.hpp>
#include <CoordServ.hpp>
#include <asio/co_spawn.hpp>
#include <iostream>

namespace server {

PeerSession::PeerSession(
    asio::ip::tcp::socket socket,
    CoordinatorServer &server
)
    : m_conn(std::move(socket)), m_server(server) {
}

void PeerSession::start() {
    asio::co_spawn(m_conn.get_executor(), read_loop(), asio::detached);
}

asio::awaitable<void> PeerSession::send_hello(
    int my_id,
    json_protocol::MessageType type
) {
    try {
        auto payload = std::make_unique<json_protocol::HelloPayload>(my_id);
        auto msg = (type == json_protocol::MessageType::REQUEST)
            ? json_protocol::Message::create_peer_hello_request(std::move(payload))
            : json_protocol::Message::create_peer_hello_response(std::move(payload));
        co_await m_conn.send_message(msg);
    } catch (const std::exception &e) {
        std::cerr << "PeerSession::send_hello failed: " << e.what() << std::endl;
    }
}

asio::awaitable<void> PeerSession::read_loop() {
    try {
        while (m_conn.is_open()) {
            auto msg = co_await m_conn.read_message();
            handle_message(msg);
        }
    } catch (const std::exception &e) {
        std::cerr << "PeerSession read error (peer "
                  << (m_peer_id ? std::to_string(*m_peer_id) : "unknown")
                  << "): " << e.what() << std::endl;
        // TODO: уведомить CoordinatorServer о разрыве,
        // чтобы он мог обнаружить смерть peer-а и запустить выборы
    }
}

void PeerSession::handle_message(const json_protocol::Message &msg) {
    if (msg.get_action() == json_protocol::Action::PEER_HELLO) {
        handle_hello(msg);
        return;
    }

    // лог
    std::cout << "[Peer "
              << (m_peer_id ? std::to_string(*m_peer_id) : "unknown")
              << "] received unhandled action"
              << std::endl;
}

void PeerSession::handle_hello(const json_protocol::Message &msg) {
    auto *payload =
        dynamic_cast<json_protocol::HelloPayload *>(msg.payload.get());
    if (payload == nullptr) {
        std::cerr << "PEER_HELLO without HelloPayload" << std::endl;
        return;
    }

    m_peer_id = payload->get_peer_id();
    std::cout << "[Peer] HELLO from peer id=" << *m_peer_id << std::endl;

    if (msg.get_type() == json_protocol::MessageType::REQUEST) {
        auto self = shared_from_this();
        asio::co_spawn(
            m_conn.get_executor(),
            [self, my_id = m_server.get_id()]() -> asio::awaitable<void> {
                co_await self->send_hello(
                    my_id, json_protocol::MessageType::RESPONSE
                );
            },
            asio::detached
        );
    }
}

}  // namespace server