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

enum class Role { Primary, Backup };

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
    
    int get_id() const { return m_id; }
    Role get_role() const { return m_role; }

    void on_peer_hello(std::shared_ptr<PeerSession> peer);


private:
    asio::io_context &m_io;
    int m_id;
    bool m_is_subtask = false;

    short m_worker_port;
    short m_client_port;
    // are worker/client acceptors open
    bool m_serving = false;

    void open_serving_acceptors();
    void close_serving_acceptors();
    void update_serving_state();

    Role m_role = Role::Backup;
    void recompute_role();

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


    // --- Heartbeat ---
    asio::steady_timer m_heartbeat_timer;
    asio::steady_timer m_initial_role_timer;

    asio::steady_timer m_primary_watch_timer;
    std::chrono::steady_clock::time_point m_last_primary_ping;
    bool m_primary_alive = false;
    bool m_grace_expired = false;

    static constexpr int HEARTBEAT_INTERVAL_SEC = 2;
    static constexpr int HEARTBEAT_TIMEOUT_SEC = 10;
    static constexpr int INITIAL_ROLE_GRACE_SEC = 15;

    void start_heartbeat_sender();
    void start_primary_watcher();
    asio::awaitable<void> send_ping_to_backups();
    void on_primary_ping();
    void on_peer_disconnected(std::shared_ptr<PeerSession> peer);
    void on_primary_dead();

    // --- Election ---
    bool m_election_in_progress = false;
    bool m_received_alive = false;
    asio::steady_timer m_election_timer;

    static constexpr int ELECTION_TIMEOUT_SEC = 3;


    void start_election();
    asio::awaitable<void> send_election_to_lower_ids();
    asio::awaitable<void> announce_coordinator();
    void on_peer_election(int from_id);
    void on_peer_alive(int from_id);
    void on_peer_coordinator(int from_id);

    asio::awaitable<void> send_alive_to(int target_id);

};

}  // namespace server
