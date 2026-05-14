// CoordinatorServer.cpp
#include <CoordServ.hpp>
#include <asio/co_spawn.hpp>
#include <cassert>
#include <iostream>
#include "Coordinator.hpp"
#include <VigenereEncryptedMessage.hpp>

namespace server {

// ---------- CoordinatorServer ----------

CoordinatorServer::CoordinatorServer(
    asio::io_context &io,
    short worker_port,
    short client_port
)
    : m_io(io),
      m_worker_acceptor(
          io,
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), worker_port)
      ),
      m_client_acceptor(
          io,
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), client_port)
      ),
      m_check_timer(io) {
    start_timeout_checker();
}

void CoordinatorServer::start() {
    // Запускаем корутины для приёма подключений
    asio::co_spawn(m_io, worker_accept_loop(), asio::detached);
    asio::co_spawn(m_io, client_accept_loop(), asio::detached);
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

void CoordinatorServer::add_worker(std::shared_ptr<WorkerSession> worker) {
    m_workers.push_back(worker);
    std::cout << "Worker connected. Total workers: " << m_workers.size()
              << std::endl;
    if (m_coordinator && m_coordinator->has_unassigned_units()) {
        asio::co_spawn(
            m_io.get_executor(),
            m_coordinator->assign_to_worker(worker),
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
    std::shared_ptr<Policy> policy
) {

    m_coordinator = std::make_unique<Coordinator>(message, policy);
    // Если есть чекпоинт, то восстанавливаемся из него
    if (m_coordinator->load_checkpoint(m_checkpoint_path, message->get_text())) {
        m_last_checkpoint_done = m_coordinator->done_units_count();
        std::cout << "Restored from checkpoint: "
                  << m_last_checkpoint_done << "/"
                  << m_coordinator->unit_count() << " units done" << std::endl;
    } else {
        m_last_checkpoint_done = 0;
        std::cout << "New task created. Units: "
                  << m_coordinator->unit_count() << std::endl;
    }

    // Попытаться назначить юниты доступным воркерам
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

void ClientSession::handle_task_request(const json_protocol::Message &msg) {
    if (msg.get_type() == json_protocol::MessageType::REQUEST &&
        msg.get_action() == json_protocol::Action::DECRYPT) {
        auto *payload =
            dynamic_cast<json_protocol::DecryptPayload *>(msg.payload.get());
        if (payload == nullptr) {
            return;
        }

        std::shared_ptr<EncryptedMessage> encrypted_msg;

        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            m_current_cipher = payload->get_cipher();
            auto cipher_vec = payload->get_cipher_text();
            std::string text(cipher_vec.begin(), cipher_vec.end());
            std::cout << "DEBUG: text2 = " << text << std::endl;
            encrypted_msg = std::make_shared<CaesarEncryptedMessage>(text);
        }else if (payload->get_cipher() == decrypt::CipherType::VIGENERE) {
            m_current_cipher = payload->get_cipher();
            auto cipher_vec = payload->get_cipher_text();
            std::string text(cipher_vec.begin(), cipher_vec.end());
            encrypted_msg = std::make_shared<VigenereEncryptedMessage>(text);
        }
        else {
            std::cerr << "Unsupported cipher" << std::endl;
            return;
        }

        int start =
            payload->get_start_key().empty() ? 0 : payload->get_start_key()[0];
        int end =
            payload->get_end_key().empty() ? 25 : payload->get_end_key()[0];
        double noise = payload->get_noise();
        decrypt::CipherType cipher = payload->get_cipher();
        decrypt::VigenereMode mode = payload->get_mode();
        std::cout << "NOISE FROM CLIENT = " << noise << std::endl;
        std::cout << "DEBUG: cipher_keys = " << start << ' ' << end
                  << std::endl;

        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            // TODO: нужно добавить возможность менять размер юнита
            auto policy = std::make_shared<StaticPolicy>(26, 5, noise, cipher, mode, 1);
            m_server.set_task(encrypted_msg, policy);
        }
        else if (payload->get_cipher() == decrypt::CipherType::VIGENERE) {
            const int key_len = payload->get_key_length();
            const int total = std::pow(26, key_len);

            auto policy = std::make_shared<StaticPolicy>(
                total, 1000, noise, cipher, mode, key_len
            );
            m_server.set_task(encrypted_msg, policy);
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
    std::cout << "⏰ Таймаут юнита " << unit_index << " (не выполнен за 3 сек)"
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
    bool should_save = (done - m_last_checkpoint_done >= CHECKPOINT_EVERY_N_UNITS)
                       || (m_coordinator->all_units_done() && done > m_last_checkpoint_done);
    if (should_save) {
        m_coordinator->save_checkpoint(m_checkpoint_path);
        m_last_checkpoint_done = done;
        std::cout << "Checkpoint saved at " << done << "/"
                  << m_coordinator->unit_count() << " done units" << std::endl;
    }
}

}  // namespace server
