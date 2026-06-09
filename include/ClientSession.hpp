#pragma once

#include <asio.hpp>
#include <deque>
#include <memory>
#include <vector>
#include "CaesarEncryptedMessage.hpp"
#include "JSON_Protocol.hpp"
#include "StaticPolicy.hpp"
#include "Unit.hpp"

namespace server {
class CoordinatorServer;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(asio::ip::tcp::socket socket, CoordinatorServer &server);
    void start();

    void post_progress_update(
        const Result &best,
        int progress_percent,
        std::size_t done_units,
        std::size_t total_units,
        std::size_t leased_units,
        std::size_t workers,
        bool finished
    );

    void post_final_result(const Result &result);

private:
    struct ProgressSnapshot {
        Result best;
        int progress_percent = 0;
        std::size_t done_units = 0;
        std::size_t total_units = 0;
        std::size_t leased_units = 0;
        std::size_t workers = 0;
        bool finished = false;
    };

    json_protocol::Connection m_conn;
    CoordinatorServer &m_server;
    decrypt::CipherType m_current_cipher = decrypt::CipherType::UNKNOWN;

    std::deque<ProgressSnapshot> m_send_queue;
    bool m_send_pump_active = false;

    void enqueue_snapshot(ProgressSnapshot snap);
    void pump_send_queue();
    asio::awaitable<void> send_progress_snapshot(const ProgressSnapshot &snap);

    asio::awaitable<void> read_loop();
    asio::awaitable<void> handle_task_request(const json_protocol::Message &msg);
};

}  // namespace server
