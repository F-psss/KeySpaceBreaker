// CoordinatorServer.cpp
#include <CoordServ.hpp>
#include <asio/co_spawn.hpp>
#include <iostream>

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
      ) {
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
    std::cout << "New task created. Units: " << m_coordinator->unit_count()
              << std::endl;
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

    asio::co_spawn(
        m_io,
        m_client->send_final_result(result),
        asio::detached
    );
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

        // auto cipher_vec = payload->get_cipher_text();
        // std::cout << "DEBUG: cipher_text size = " << cipher_vec.size()
        //         << std::endl;
        // std::string text(cipher_vec.begin(), cipher_vec.end());

        // std::cout << "DEBUG: text = " << text << std::endl;

        if (payload->get_cipher() == decrypt::CipherType::CAESAR) {
            std::cout << "DEBUG: textrrr2 = " << std::endl;
            auto cipher_vec = payload->get_cipher_text();
            std::string text(cipher_vec.begin(), cipher_vec.end());
            std::cout << "DEBUG: text2 = " << text << std::endl;
            encrypted_msg = std::make_shared<CaesarEncryptedMessage>(text);
        } else {
            std::cerr << "Unsupported cipher" << std::endl;
            return;
        }

        int start =
            payload->get_start_key().empty() ? 0 : payload->get_start_key()[0];
        int end =
            payload->get_end_key().empty() ? 25 : payload->get_end_key()[0];

        std::cout << "DEBUG: cipher_keys = " << start << ' ' << end
                  << std::endl;

        auto policy = std::make_shared<StaticPolicy>(26, 5);

        m_server.set_task(encrypted_msg, policy);
    }
}

}  // namespace server
