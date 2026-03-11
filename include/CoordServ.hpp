#pragma once

#include <ClientSession.hpp>
#include "Coordinator.hpp"
#include "EncryptedMessage.hpp"
#include "JSON_Protocol.hpp"
#include "Policy.hpp"

namespace server {

class CoordinatorServer {
    friend class WorkerSession;

public:
    CoordinatorServer(
        asio::io_context &io,
        short worker_port,
        short client_port
    );

    // Запуск акцепторов (корутины)
    void start();

    // Управление воркерами
    void add_worker(std::shared_ptr<WorkerSession> worker);
    void remove_worker(std::shared_ptr<WorkerSession> worker);

    // Установка новой задачи от клиента
    void set_task(
        std::shared_ptr<EncryptedMessage> message,
        std::shared_ptr<Policy> policy
    );

    // Отправить юнит воркеру (вызывается из Coordinator)
    void assign_unit_to_worker(
        std::shared_ptr<Unit> unit,
        std::shared_ptr<WorkerSession> worker
    );

private:
    asio::io_context &m_io;
    asio::ip::tcp::acceptor m_worker_acceptor;
    asio::ip::tcp::acceptor m_client_acceptor;

    std::vector<std::shared_ptr<WorkerSession>> m_workers;
    std::unique_ptr<ClientSession> m_client;
    std::unique_ptr<Coordinator> m_coordinator;

    // Корутины для приёма подключений
    asio::awaitable<void> worker_accept_loop();
    asio::awaitable<void> client_accept_loop();
};

}  // namespace server
