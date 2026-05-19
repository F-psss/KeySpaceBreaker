#ifndef JSON_PROTOCOL_HPP
#define JSON_PROTOCOL_HPP

#include <asio.hpp>
#include "enums.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace json_protocol {

[[nodiscard]] std::string action_to_string(Action action);
[[nodiscard]] Action string_to_action(const std::string &str);
[[nodiscard]] std::string type_to_string(MessageType type);
[[nodiscard]] MessageType string_to_type(const std::string &str);
[[nodiscard]] std::string cipher_to_string(decrypt::CipherType type);
[[nodiscard]] decrypt::CipherType string_to_cipher(const std::string &str);

static constexpr uint32_t MAX_MESSAGE_SIZE = 1000 * 1024;  // 1000KB
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
    DecryptPayload()
    : cipher(decrypt::CipherType::UNKNOWN),
      mode(decrypt::VigenereMode::UNKNOWN),
      noise(0.5),
      key_length(0)
{}
    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

    [[nodiscard]] decrypt::CipherType get_cipher() const {
        return cipher;
    }

    [[nodiscard]] const std::vector<uint8_t> &get_cipher_text() const {
        return cipher_text;
    }

    [[nodiscard]] const std::vector<uint8_t> &get_start_key() const {
        return start_key;
    }

    [[nodiscard]] const std::vector<uint8_t> &get_end_key() const {
        return end_key;
    }
    [[nodiscard]] decrypt::VigenereMode get_mode() const {
        return mode;
    }
    [[nodiscard]] int get_key_length() const {
        return key_length;
    }
    [[nodiscard]] double get_noise() const {
        return noise;
    }
    void set_cipher(decrypt::CipherType ciphe) {
        cipher = ciphe;
    }

    void set_cipher_text(const std::vector<uint8_t> &cipher_tex) {
        cipher_text = cipher_tex;
    }

    void set_start_key(const std::vector<uint8_t> &start_ke) {
        start_key = start_ke;
    }

    void set_end_key(const std::vector<uint8_t> &end_ke) {
        end_key = end_ke;
    }

    void set_noise(double n) {
        noise = n;
    }
    void set_mode(decrypt::VigenereMode m) {
        mode = m;
    }
    void set_key_length(int key_l) {
        key_length = key_l;
    }

private:
    decrypt::CipherType cipher;
    decrypt::VigenereMode mode;
    std::vector<uint8_t> cipher_text;
    double noise;
    std::vector<uint8_t> start_key;
    std::vector<uint8_t> end_key;
    int key_length;
};

class StatusPayload final : public Payload {  // TODO
public:
    StatusPayload() = default;

    StatusPayload(
        decrypt::CipherType cipher,
        std::string cipher_text,
        std::string key,
        double score
    )
        : m_cipher(cipher),
          m_cipher_text(std::move(cipher_text)),
          m_key(key),
          m_score(score) {
    }

    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

    [[nodiscard]] std::string get_cipher_text() const {
        return m_cipher_text;
    }

    [[nodiscard]] std::string get_key() const {
        return m_key;
    }

    [[nodiscard]] double get_score() const {
        return m_score;
    }

private:
    decrypt::CipherType m_cipher = decrypt::CipherType::UNKNOWN;
    std::string m_cipher_text;
    std::string m_key;
    double m_score;
    int m_progress;
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
        : payload(std::move(p)), type(t), action(a) {
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
    static Message create_peer_hello_request(std::unique_ptr<Payload>);
    static Message create_peer_hello_response(std::unique_ptr<Payload>);
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

class HelloPayload final : public Payload {
public:
    HelloPayload() = default;
    explicit HelloPayload(int peer_id) : m_peer_id(peer_id) {}

    [[nodiscard]] json to_json() const final;
    [[nodiscard]] std::unique_ptr<Payload> from_json(const json &) const final;

    [[nodiscard]] int get_peer_id() const {
        return m_peer_id;
    }

    void set_peer_id(int peer_id) {
        m_peer_id = peer_id;
    }

private:
    int m_peer_id = 0;
};


};  // namespace json_protocol
#endif
