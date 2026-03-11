#ifndef NETWORKWORKER_HPP
#define NETWORKWORKER_HPP

#include <Decryptor.hpp>
#include <EncryptedMessage.hpp>
#include <JSON_Protocol.hpp>
#include <iostream>

class Worker {
public:
    Worker(asio::io_context &io, std::string ip, uint16_t port)
        : m_io(io), m_coordinator_ip(std::move(ip)), m_coordinator_port(port) {
    }

    void start();
    void stop();

private:
    asio::awaitable<void> run_decryptor_task(std::shared_ptr<server::Unit> unit
    );
    asio::awaitable<void> connect();
    asio::awaitable<void> run();
    void handle_message(const json_protocol::Message &msg);

    asio::io_context &m_io;
    std::unique_ptr<server::Decryptor> m_decryptor;
    std::string m_coordinator_ip = "127.0.0.1";
    uint16_t m_coordinator_port = 12345;
    std::shared_ptr<json_protocol::Connection> m_conn;
};
#endif
