#ifndef JSON_PROTOCOL_HPP
#define JSON_PROTOCOL_HPP

#include <asio.hpp>
#include <enums.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace json_protocol {

[[nodiscard]] std::string action_to_string(Action action);
[[nodiscard]] Action string_to_action(const std::string &str);
[[nodiscard]] std::string type_to_string(MessageType type);
[[nodiscard]] MessageType string_to_type(const std::string &str);
[[nodiscard]] std::string cipher_to_string(decrypt::CipherType type);
[[nodiscard]] decrypt::CipherType string_to_cipher(const std::string &str);

static constexpr uint32_t MAX_MESSAGE_SIZE = 10 * 1024;  // 10KB
static constexpr uint32_t HEADER_SIZE = 32;

class Payload {
public:
    Payload(const Payload &) = delete;
    Payload &operator=(const Payload &) = delete;
    Payload(Payload &&) = default;
    Payload &operator=(Payload &&) = default;
    virtual ~Payload() = default;

    [[nodiscard]] virtual json to_json() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Payload> from_json(const json &)
        const = 0;

protected:
    Payload() = default;

private:
    json body;
};

class DecryptPayload final : public Payload {
public:
    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

private:
    decrypt::CipherType cipher;
    std::string cipher_text;  // TODO
    std::string start_key;    // TODO
    std::string end_key;      // TODO
};

class StatusPayload final : public Payload {  // TODO
public:
    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

private:
    decrypt::CipherType cipher;
    std::string cipher_text;
    int progress;
};

class PingPayload final : public Payload {  // TODO
public:
    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

    [[nodiscard]] int get_code() const {
        return code;
    }

    void set_code(int cod) {
        code = cod;
    }

private:
    int code = 200;
};

class QuitPayload final : public Payload {  // TODO
public:
    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

private:
    int code = 300;
};

class Message {
public:
    Message() = default;

    Message(MessageType t, Action a, std::unique_ptr<Payload> p)
        : type(t), action(a), payload(std::move(p)) {
    }

    Message(const Message &) = delete;
    Message &operator=(const Message &) = delete;
    Message(Message &&) = default;
    Message &operator=(Message &&) = default;
    ~Message() = default;

    [[nodiscard]] MessageType get_type() const {
        return type;
    }

    [[nodiscard]] Action get_action() const {
        return action;
    }

    [[nodiscard]] json to_json() const;
    static Message from_json(const json &j);

    static Message create_decrypt_request(std::unique_ptr<Payload>);
    static Message create_decrypt_response(std::unique_ptr<Payload>);
    static Message create_status_request(std::unique_ptr<Payload>);
    static Message create_status_response(std::unique_ptr<Payload>);
    static Message create_ping_request(std::unique_ptr<Payload>);
    static Message create_pong_response(std::unique_ptr<Payload>);

    std::unique_ptr<Payload> payload;

private:
    MessageType type = MessageType::UNKNOWN;
    Action action = Action::UNKNOWN;
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
};  // namespace json_protocol
#endif
