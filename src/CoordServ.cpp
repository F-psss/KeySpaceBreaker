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
      m_checkpoint_path(std::move(checkpoint_path)) {
    start_timeout_checker();
}

std::string crop_text(const std::string& text) {
    int counter_words = 0;
    std::string cropped_text;
    for (auto c: text) {
        if (c == ' ') {
            counter_words++;
        }
        cropped_text += c;
        if (counter_words == 200) {
            break;
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
                std::make_shared<PeerSession>(std::move(socket), *this);
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
                std::make_shared<PeerSession>(std::move(socket), *this);
            add_peer(session);
            session->start();

            co_await session->send_hello(
                m_id, json_protocol::MessageType::REQUEST
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
            std::cout << "mode = " << (int)mode << " cipher = " << (int)cipher << std::endl;
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
                total, 1000, noise, cipher, mode, key_len
            );
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

}  // namespace server
