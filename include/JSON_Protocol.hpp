#ifndef JSON_PROTOCOL_HPP
#define JSON_PROTOCOL_HPP

#include <asio.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonProtocol {
public:
    static constexpr uint32_t MAX_MESSAGE_SIZE = 10 * 1024;  // 10KB
    static constexpr uint32_t HEADER_SIZE = 32;

    struct Message {
        [[nodiscard]] json to_json() const;
        static Message from_json(const json &j);

        static Message create_request(
            const std::string &action,
            const json &payload = {},
            const json &metadata = {}
        );
        static Message create_response(
            const std::string &action,
            const json &payload = {},
            const std::string &status = "success"
        );
        std::string type;
        std::string action;
        json payload;
        json metadata;
        std::string status = "success";
    };

    class Connection {
    public:
        Connection(asio::ip::tcp::socket socket) : socket_(std::move(socket)) {
        }

        asio::awaitable<void> send_message(const Message &msg);
        asio::awaitable<Message> read_message();

        void close() {
            if (socket_.is_open()) {
                asio::error_code ec;
                socket_.close(ec);
            }
        }

        [[nodiscard]] bool is_open() const {
            return socket_.is_open();
        }

        auto get_executor() {
            return socket_.get_executor();
        }

    private:
        asio::ip::tcp::socket socket_;
    };
};
#endif
