#pragma once

#include <ClientSession.hpp>
#include <PeerSession.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Coordinator.hpp"
#include "EncryptedMessage.hpp"
#include "JSON_Protocol.hpp"
#include "Policy.hpp"

namespace server {

class CoordinatorServer {
    friend class WorkerSession;
    friend class PeerSession;

public:
    CoordinatorServer(
        asio::io_context &io,
        short worker_port,
        short client_port,
        short peer_potr,
        int id,
        std::vector<std::string> peer_addresses,
        std::string checkpoint_path
    );

    // Запуск акцепторов (корутины)
    void start();
    bool is_subtask() const { return m_is_subtask; }
    // Управление воркерами
    void add_worker(std::shared_ptr<WorkerSession> worker);
    void remove_worker(std::shared_ptr<WorkerSession> worker);

    // Установка новой задачи от клиента
    void set_task(
        std::shared_ptr<EncryptedMessage> message,
        std::shared_ptr<Policy> policy,
        decrypt::VigenereMode mode
    );

    void send_result_to_client(const Result &result);

    asio::awaitable<std::string> run_subtask(
        std::shared_ptr<EncryptedMessage> message,
        std::shared_ptr<Policy> policy
    );

    int get_id() const {
        return m_id;
    }

private:
    asio::io_context &m_io;
    int m_id;
    bool m_is_subtask = false;

    asio::ip::tcp::acceptor m_worker_acceptor;
    asio::ip::tcp::acceptor m_client_acceptor;
    asio::ip::tcp::acceptor m_peer_acceptor;

    std::vector<std::shared_ptr<WorkerSession>> m_workers;
    std::unique_ptr<ClientSession> m_client;
    std::unique_ptr<Coordinator> m_coordinator;

    std::vector<std::string> m_peer_addresses;
    std::vector<std::shared_ptr<PeerSession>> m_peers;

    // Корутины для приёма подключений
    asio::awaitable<void> worker_accept_loop();
    asio::awaitable<void> client_accept_loop();
    asio::awaitable<void> peer_accept_loop();
    asio::awaitable<void> connect_to_peer(std::string address);

    asio::steady_timer m_check_timer;

    void add_peer(std::shared_ptr<PeerSession> peer);
    void remove_peer(std::shared_ptr<PeerSession> peer);

    void start_timeout_checker();
    void check_timeouts();
    void handle_timeout(int unit_index);

    std::string m_checkpoint_path;
    std::size_t m_last_checkpoint_done = 0;
    static constexpr std::size_t CHECKPOINT_EVERY_N_UNITS = 5;
    void maybe_save_checkpoint();
};

}  // namespace server
