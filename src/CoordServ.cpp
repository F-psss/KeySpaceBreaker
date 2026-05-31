// CoordinatorServer.cpp
#include <CoordServ.hpp>
#include <VigenereEncryptedMessage.hpp>
#include <asio/co_spawn.hpp>
#include <cassert>
#include <iostream>
#include "Coordinator.hpp"
#include "KasiskiAnalyzer.hpp"

namespace server {

// ---------- CoordinatorServer ----------
CoordinatorServer::CoordinatorServer(
    asio::io_context &io,
    short worker_port,
    short client_port,
    short peer_port,
    int id,
    std::vector<std::string> peer_addresses,
    std::string checkpoint_path
)
    : m_io(io),
      m_id(id),
      m_worker_acceptor(
          io,
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), worker_port)
      ),
      m_client_acceptor(
          io,
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), client_port)
      ),
      m_peer_acceptor(
          io,
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), peer_port)
      ),
      m_peer_addresses(std::move(peer_addresses)),
      m_check_timer(io),
      m_checkpoint_path(std::move(checkpoint_path)),
      m_heartbeat_timer(io),
      m_initial_role_timer(io),
      m_primary_watch_timer(io),
      m_last_primary_ping(std::chrono::steady_clock::now()),
      m_election_timer(io) {
    start_timeout_checker();
}

std::string crop_text(const std::string &text) {
    int counter_chars = 0;
    std::string cropped_text;
    for (auto c : text) {
        cropped_text += c;
        if (std::isalpha(c)) {
            counter_chars++;
            if (counter_chars == 1000) {
                break;
            }
        }
    }
    return cropped_text;
}

void CoordinatorServer::start() {
    asio::co_spawn(m_io, worker_accept_loop(), asio::detached);
    asio::co_spawn(m_io, client_accept_loop(), asio::detached);
    asio::co_spawn(m_io, peer_accept_loop(), asio::detached);

    for (const auto &addr : m_peer_addresses) {
        asio::co_spawn(m_io, connect_to_peer(addr), asio::detached);
    }
    start_heartbeat_sender();
    start_primary_watcher();

    if (m_peer_addresses.empty()) {
        // Одиночный запуск — ждать некого, сразу определяем роль
        m_grace_expired = true;
        recompute_role();}
    else{
        m_initial_role_timer.expires_after(std::chrono::seconds(INITIAL_ROLE_GRACE_SEC));
        m_initial_role_timer.async_wait([this](std::error_code ec) {
            if (!ec) {
                m_grace_expired = true;
                recompute_role();
            }
        });
    }
}

asio::awaitable<void> CoordinatorServer::worker_accept_loop() {
    while (true) {
        try {
            auto socket =
                co_await m_worker_acceptor.async_accept(asio::use_awaitable);
            auto session =
                std::make_shared<WorkerSession>(std::move(socket), *this);
            add_worker(session);
            session->start();
        } catch (const std::exception &e) {
            std::cerr << "Worker accept error: " << e.what() << std::endl;
            // При необходимости можно выйти из цикла, если ошибка фатальная
        }
    }
}

asio::awaitable<void> CoordinatorServer::client_accept_loop() {
    while (true) {
        try {
            std::cout << "Waiting for client..." << std::endl;

            auto socket =
                co_await m_client_acceptor.async_accept(asio::use_awaitable);

            auto session =
                std::make_unique<ClientSession>(std::move(socket), *this);

            session->start();
            m_client = std::move(session);

            std::cout << "Client connected" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Client accept error: " << e.what() << std::endl;
        }
    }
}

asio::awaitable<void> CoordinatorServer::peer_accept_loop() {
    while (true) {
        try {
            auto socket =
                co_await m_peer_acceptor.async_accept(asio::use_awaitable);
            std::cout << "Peer connection accepted (incoming)" << std::endl;

            auto session =
                std::make_shared<PeerSession>(std::move(socket), *this, false);
            add_peer(session);
            session->start();

        } catch (const std::exception &e) {
            std::cerr << "Peer accept error: " << e.what() << std::endl;
        }
    }
}

asio::awaitable<void> CoordinatorServer::connect_to_peer(std::string address) {
    auto pos = address.find(':');
    if (pos == std::string::npos) {
        std::cerr << "Invalid peer address: " << address << std::endl;
        co_return;
    }
    std::string host = address.substr(0, pos);
    std::string port_str = address.substr(pos + 1);

    while (true) {
        bool need_retry = false;
        try {
            asio::ip::tcp::socket socket(m_io);
            asio::ip::tcp::resolver resolver(m_io);
            auto endpoints = co_await resolver.async_resolve(
                host, port_str, asio::use_awaitable
            );
            co_await asio::async_connect(
                socket, endpoints, asio::use_awaitable
            );

            std::cout << "Connected to peer " << address << std::endl;

            auto session =
                std::make_shared<PeerSession>(std::move(socket), *this, true);
            add_peer(session);
            session->start();

            co_await session->send_hello(
                m_id, (m_role == Role::Primary) ? 0 : 1,
                json_protocol::MessageType::REQUEST
            );

            co_return;
        } catch (const std::exception &e) {
            std::cerr << "Failed to connect to peer " << address << ": "
                      << e.what() << " — retrying in 3s" << std::endl;
            need_retry = true;
        }

        if (need_retry) {
            asio::steady_timer timer(m_io);
            timer.expires_after(std::chrono::seconds(3));
            co_await timer.async_wait(asio::use_awaitable);
        }
    }
}

void CoordinatorServer::add_peer(std::shared_ptr<PeerSession> peer) {
    m_peers.push_back(peer);
    std::cout << "Total peers: " << m_peers.size() << std::endl;
}

void CoordinatorServer::remove_peer(std::shared_ptr<PeerSession> peer) {
    auto it = std::find(m_peers.begin(), m_peers.end(), peer);
    if (it != m_peers.end()) {
        m_peers.erase(it);
        std::cout << "Peer disconnected. Remaining: " << m_peers.size()
                  << std::endl;
    }
}

void CoordinatorServer::add_worker(std::shared_ptr<WorkerSession> worker) {
    m_workers.push_back(worker);
    std::cout << "Worker connected. Total workers: " << m_workers.size()
              << std::endl;
    if (m_coordinator && m_coordinator->has_unassigned_units()) {
        asio::co_spawn(
            m_io.get_executor(), m_coordinator->assign_to_worker(worker),
            asio::detached
        );
    }
}

void CoordinatorServer::remove_worker(std::shared_ptr<WorkerSession> worker) {
    auto it = std::find(m_workers.begin(), m_workers.end(), worker);
    if (it != m_workers.end()) {
        m_workers.erase(it);
        std::cout << "Worker disconnected. Remaining: " << m_workers.size()
                  << std::endl;
    }
}

void CoordinatorServer::set_task(
    std::shared_ptr<EncryptedMessage> message,
    std::shared_ptr<Policy> policy,
    decrypt::VigenereMode mode
) {
    m_coordinator = std::make_unique<Coordinator>(message, policy);
    m_coordinator->set_mode(mode);
    // Если есть чекпоинт, то восстанавливаемся из него
    if (m_coordinator->load_checkpoint(
            m_checkpoint_path, message->get_text()
        )) {
        m_last_checkpoint_done = m_coordinator->done_units_count();
        std::cout << "Restored from checkpoint: " << m_last_checkpoint_done
                  << "/" << m_coordinator->unit_count() << " units done"
                  << std::endl;
    } else {
        m_last_checkpoint_done = 0;
        std::cout << "New task created. Units: " << m_coordinator->unit_count()
                  << std::endl;
    }

    // Попытаться назначить юниты доступным воркерам
    std::cout << "m_workers size = " << m_workers.size() << '\n';
    for (auto &worker : m_workers) {
        asio::co_spawn(
            m_io.get_executor(), m_coordinator->assign_to_worker(worker),
            asio::detached
        );
    }
}

void CoordinatorServer::send_result_to_client(const Result &result) {
    if (!m_client) {
        std::cerr << "No client connected to send result\n";
        return;
    }

    asio::co_spawn(m_io, m_client->send_final_result(result), asio::detached);
}

asio::awaitable<void> ClientSession::handle_task_request(
    const json_protocol::Message &msg
) {
    if (msg.get_type() == json_protocol::MessageType::REQUEST &&
        msg.get_action() == json_protocol::Action::DECRYPT) {
        auto *payload =
            dynamic_cast<json_protocol::DecryptPayload *>(msg.payload.get());
        if (payload == nullptr) {
            co_return;
        }

        std::shared_ptr<EncryptedMessage> encrypted_msg;

        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            m_current_cipher = payload->get_cipher();
            auto cipher_vec = payload->get_cipher_text();
            std::string text(cipher_vec.begin(), cipher_vec.end());
            std::cout << "DEBUG: text2 = " << text << std::endl;
            encrypted_msg = std::make_shared<CaesarEncryptedMessage>(text);
        } else if (payload->get_cipher() == decrypt::CipherType::VIGENERE) {
            m_current_cipher = payload->get_cipher();
            auto cipher_vec = payload->get_cipher_text();
            std::string text(cipher_vec.begin(), cipher_vec.end());
            if (payload->get_mode() == decrypt::VigenereMode::FAST) {
                text = crop_text(text);
            }
            encrypted_msg = std::make_shared<VigenereEncryptedMessage>(text);
        } else {
            std::cerr << "Unsupported cipher" << std::endl;
            co_return;
        }

        int start =
            payload->get_start_key().empty() ? 0 : payload->get_start_key()[0];
        int end =
            payload->get_end_key().empty() ? 25 : payload->get_end_key()[0];
        const double noise = payload->get_noise();
        decrypt::CipherType cipher = payload->get_cipher();
        decrypt::VigenereMode mode = payload->get_mode();
        std::cout << "NOISE FROM CLIENT = " << noise << std::endl;
        std::cout << "DEBUG: cipher_keys = " << start << ' ' << end
                  << std::endl;

        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            // TODO: нужно добавить возможность менять размер юнита
            auto policy =
                std::make_shared<StaticPolicy>(26, 5, noise, cipher, mode, 1);
            m_server.set_task(encrypted_msg, policy, mode);
        } else if (payload->get_cipher() == decrypt::CipherType::VIGENERE) {
            std::string full_ciphertext(
                payload->get_cipher_text().begin(),
                payload->get_cipher_text().end()
            );
            int key_len = payload->get_key_length();
            std::cout << "key_len = " << key_len << '\n';
            if (key_len < 1 || key_len > 7) {
                key_len =
                    KasiskiAnalyzer::guessKeyLength(full_ciphertext, 2, 7);
            }
            std::cout << "key_len = " << key_len << '\n';
            const int total = std::pow(26, key_len);
            auto policy = std::make_shared<StaticPolicy>(
                total, 10000, noise, cipher, mode, key_len
            );
            if (payload->get_mode() == decrypt::VigenereMode::FAST) {
                policy = std::make_shared<StaticPolicy>(
                    total, 10000, noise, cipher, mode, key_len
                );
            }
            m_server.set_task(encrypted_msg, policy, mode);
        }
    }
}

void CoordinatorServer::start_timeout_checker() {
    m_check_timer.expires_after(std::chrono::seconds(1));
    m_check_timer.async_wait([this](std::error_code ec) {
        if (!ec) {
            check_timeouts();
        }
    });
}

void CoordinatorServer::check_timeouts() {
    if (m_coordinator) {
        // assert(false);
        auto now = std::chrono::steady_clock::now();
        for (auto it = m_coordinator->m_pending_units.begin();
             it != m_coordinator->m_pending_units.end();) {
            if (now - it->second.start_time >= UNIT_TIMEOUT) {
                handle_timeout(it->first);
                it = m_coordinator->m_pending_units.erase(it);
            } else {
                ++it;
            }
        }
    }

    maybe_save_checkpoint();

    start_timeout_checker();  // следующая проверка
}

void CoordinatorServer::handle_timeout(int unit_index) {
    if (!m_coordinator) {
        return;
    }
    std::cout << "⏰ Таймаут юнита " << unit_index << " (не выполнен за 10 сек)"
              << std::endl;
    m_coordinator->mark_as_unassigned_by_index(unit_index);
    remove_worker(m_coordinator->m_pending_units[unit_index].worker);
    for (auto &worker : m_workers) {
        if (worker->is_free()) {
            asio::co_spawn(
                m_io.get_executor(), m_coordinator->assign_to_worker(worker),
                asio::detached
            );
        }
    }

    // TODO сделать переностакх воркеров в отдельный списо
}

void CoordinatorServer::maybe_save_checkpoint() {
    if (!m_coordinator) {
        return;
    }
    std::size_t done = m_coordinator->done_units_count();
    bool should_save =
        (done - m_last_checkpoint_done >= CHECKPOINT_EVERY_N_UNITS) ||
        (m_coordinator->all_units_done() && done > m_last_checkpoint_done);
    if (should_save) {
        m_coordinator->save_checkpoint(m_checkpoint_path);
        m_last_checkpoint_done = done;
        std::cout << "Checkpoint saved at " << done << "/"
                  << m_coordinator->unit_count() << " done units" << std::endl;
    }
}

void CoordinatorServer::on_peer_hello(std::shared_ptr<PeerSession> peer) {
    if (!peer->peer_id().has_value()) {
        return;
    }

    int pid = peer->peer_id().value();
    int prole = peer->peer_role();

    

    for (auto &other : m_peers) {
        if (other == peer) {
            continue;
        }
        if (!other->peer_id().has_value()) {
            continue;
        }
        if (other->peer_id().value() != pid) {
            continue;
        }

        int initiator_of_peer = peer->is_initiator() ? m_id : pid;
        int initiator_of_other = other->is_initiator() ? m_id : pid;

        if (initiator_of_peer < initiator_of_other) {
            other->close_intentionally();
            remove_peer(other);
        } else {
            peer->close_intentionally();
            remove_peer(peer);
        }
        break;
    }

    if (!peer->is_open()) {
        return;
    }

    if (prole == 0 && m_role == Role::Primary) {
        if (pid < m_id) {
            m_role = Role::Backup;
            m_primary_alive = true;
            m_last_primary_ping = std::chrono::steady_clock::now();
        } else {
            asio::co_spawn(m_io, announce_coordinator(), asio::detached);
        }
        return;
    }

    recompute_role();
}

void CoordinatorServer::recompute_role() {
    if (m_role == Role::Primary) {
        return;
    }

    bool exists_primary_peer = false;
    int min_alive_id = m_id;
    int known_peers = 0;

    for (auto &p : m_peers) {
        if (!p->peer_id().has_value()) continue;
        known_peers++;
        int pid = p->peer_id().value();
        min_alive_id = std::min(min_alive_id, pid);
        if (p->peer_role() == 0) {
            exists_primary_peer = true;
        }
    }
    if (!m_grace_expired &&
        known_peers < static_cast<int>(m_peer_addresses.size())) {
        return;
    }

    if (!exists_primary_peer && m_id == min_alive_id) {
        m_role = Role::Primary;
        m_primary_alive = true;
        std::cout << "Role changed to PRIMARY" << std::endl;
        asio::co_spawn(m_io, announce_coordinator(), asio::detached);
        // TODO: открыть/закрыть worker и client акцепторы в зависимости от роли
    }
}

void CoordinatorServer::start_heartbeat_sender() {
    m_heartbeat_timer.expires_after(std::chrono::seconds(HEARTBEAT_INTERVAL_SEC)
    );
    m_heartbeat_timer.async_wait([this](std::error_code ec) {

        if (ec) {
            return;
        }
        if (m_role == Role::Primary) {
            asio::co_spawn(m_io, send_ping_to_backups(), asio::detached);
        }
        start_heartbeat_sender();
    });
}

asio::awaitable<void> CoordinatorServer::send_ping_to_backups() {
    auto peers_copy = m_peers;
    for (auto &peer : peers_copy) {
        if (!peer->is_open()) {
            continue;
        }
        auto payload = std::make_unique<json_protocol::PingPayload>();
        auto msg = json_protocol::Message::create_peer_ping(std::move(payload));
        co_await peer->send_raw(msg);
    }
    co_return;
}

void CoordinatorServer::on_primary_ping() {
    m_last_primary_ping = std::chrono::steady_clock::now();
    if (!m_primary_alive) {
        m_primary_alive = true;
        std::cout << "Primary heartbeat detected" << std::endl;
    }
}

void CoordinatorServer::start_primary_watcher() {
    m_primary_watch_timer.expires_after(std::chrono::seconds(1));
    m_primary_watch_timer.async_wait([this](std::error_code ec) {
        if (ec) {
            return;
        }
        if (m_role == Role::Backup) {
            auto now = std::chrono::steady_clock::now();
            auto silence = std::chrono::duration_cast<std::chrono::seconds>(
                               now - m_last_primary_ping
            )
                               .count();
            if (m_primary_alive && silence > HEARTBEAT_TIMEOUT_SEC) {
                m_primary_alive = false;
                on_primary_dead();
            }
        }
        start_primary_watcher();
    });
}

void CoordinatorServer::on_peer_disconnected(std::shared_ptr<PeerSession> peer) {
    int pid = peer->peer_id().value_or(-1);
    int prole = peer->peer_role();

    std::cout << "Peer " << pid << " disconnected (role="
              << (prole == 0 ? "Primary" : "Backup") << ")" << std::endl;

    remove_peer(peer);

    if (prole == 0 && m_role == Role::Backup && m_primary_alive) {
        m_primary_alive = false;
        std::cout << "⚠️  Primary connection lost" << std::endl;
        on_primary_dead();
    }
}

void CoordinatorServer::on_primary_dead() {
    std::cout << "⚠️  Primary is DEAD — starting election" << std::endl;
    start_election();
}

void CoordinatorServer::start_election() {
    if (m_election_in_progress) {
        return;
    }
    m_election_in_progress = true;
    m_received_alive = false;
    std::cout << "[Election] starting (by id=" << m_id << ")" << std::endl;

    
    asio::co_spawn(m_io, send_election_to_lower_ids(), asio::detached);

    // waiting for ALIVE
    m_election_timer.expires_after(std::chrono::seconds(ELECTION_TIMEOUT_SEC));
    m_election_timer.async_wait([this](std::error_code ec) {
        if (ec) return;
        if (!m_election_in_progress) return;

        if (m_received_alive) {
            
            std::cout << "[Election] received ALIVE — stepping back" << std::endl;
            m_election_in_progress = false;
        } else {
            
            std::cout << "[Election] no ALIVE received id=" << m_id << " the new PRIMARY"
                      << std::endl;
            m_election_in_progress = false;
            m_role = Role::Primary;
            m_primary_alive = true;
            asio::co_spawn(m_io, announce_coordinator(), asio::detached);
        }
    });
}

asio::awaitable<void> CoordinatorServer::send_election_to_lower_ids() {
    auto peers_copy = m_peers;
    for (auto &peer : peers_copy) {
        if (!peer->is_open()) continue;
        auto pid_opt = peer->peer_id();
        if (!pid_opt.has_value()) continue;
        if (pid_opt.value() >= m_id) continue;

        auto payload = std::make_unique<json_protocol::PeerIdPayload>(m_id);
        auto msg = json_protocol::Message::create_peer_election(std::move(payload));
        co_await peer->send_raw(msg);
    }
    co_return;
}

asio::awaitable<void> CoordinatorServer::announce_coordinator() {
    auto peers_copy = m_peers;
    for (auto &peer : peers_copy) {
        if (!peer->is_open()) continue;
        auto payload = std::make_unique<json_protocol::PeerIdPayload>(m_id);
        auto msg = json_protocol::Message::create_peer_coordinator(std::move(payload));
        co_await peer->send_raw(msg);
    }
    co_return;
}

asio::awaitable<void> CoordinatorServer::send_alive_to(int target_id) {
    for (auto &peer : m_peers) {
        if (!peer->is_open()) continue;
        if (peer->peer_id().value_or(-1) != target_id) continue;

        auto payload = std::make_unique<json_protocol::PeerIdPayload>(m_id);
        auto msg = json_protocol::Message::create_peer_alive(std::move(payload));
        co_await peer->send_raw(msg);
        co_return;
    }
}

void CoordinatorServer::on_peer_election(int from_id) {
    std::cout << "[Election] received ELECTION from id=" << from_id << std::endl;

    // Если from_id > нашего — отвечаем ALIVE и сами запускаем выборы
    if (from_id > m_id) {
        asio::co_spawn(m_io, send_alive_to(from_id), asio::detached);
        if (!m_election_in_progress) {
            start_election();
        }
    }
    // Если from_id < нашего — игнорируем (вообще не должны были получить, но на всякий)
}

void CoordinatorServer::on_peer_alive(int from_id) {
    std::cout << "[Election] received ALIVE from id=" << from_id << std::endl;
    if (m_election_in_progress) {
        m_received_alive = true;
    }
}

void CoordinatorServer::on_peer_coordinator(int from_id) {
    std::cout << "received COORDINATOR from id=" << from_id << std::endl;

    // Конфликт: мы Primary и пришёл COORDINATOR от узла с БОЛЬШИМ ID
    if (m_role == Role::Primary && from_id > m_id) {
        std::cout << "[Election] conflict: I have smaller ID, asserting myself"
                  << std::endl;
        asio::co_spawn(m_io, announce_coordinator(), asio::detached);
        return;
    }

    // Принимаем нового primary
    for (auto &peer : m_peers) {
        if (peer->peer_id().value_or(-1) == from_id) {
            peer->set_peer_role(0);  // Primary
        } else if (peer->peer_role() == 0) {
            peer->set_peer_role(1);  // был Primary — теперь Backup
        }
    }

    if (m_role == Role::Primary) {
        std::cout << "Role changed to BACKUP (yielded to id=" << from_id << ")"
                  << std::endl;
    }
    m_role = Role::Backup;
    m_election_in_progress = false;
    m_received_alive = false;
    m_primary_alive = false; // ждём первого пинга, только тогда будет true
    m_last_primary_ping = std::chrono::steady_clock::now();
}

}  // namespace server
