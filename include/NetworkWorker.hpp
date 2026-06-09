#ifndef NETWORKWORKER_HPP
#define NETWORKWORKER_HPP

#include <Decryptor.hpp>
#include <EncryptedMessage.hpp>
#include <JSON_Protocol.hpp>

class Worker {
public:
    Worker(
        asio::io_context &io,
        std::vector<std::string> coordinator_addresses,
        const std::string &dict_path,
        const std::string &trigrams_path
    )
        : m_io(io),
          m_coordinator_addresses(std::move(coordinator_addresses)),
          m_dict_path(dict_path),
          m_trigrams_path(trigrams_path) {
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
    std::vector<std::string> m_coordinator_addresses;
    std::string m_dict_path;
    std::string m_trigrams_path;
    std::shared_ptr<json_protocol::Connection> m_conn;
    std::vector<uint8_t> m_cachedCiphertext;
    bool m_hasCiphertext = false;
};

#endif  // NETWORKWORKER_HPP