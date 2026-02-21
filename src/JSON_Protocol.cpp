#include <JSON_Protocol.hpp>

using json = nlohmann::json;

namespace json_protocol {

// Функция для преобразования Action в строку
[[nodiscard]] std::string action_to_string(Action action) {
    switch (action) {
        case Action::DECRYPT:
            return "decrypt";
        case Action::STATUS:
            return "status";
        case Action::PING:
            return "ping";
        case Action::QUIT:
            return "quit";
        default:
            return "unknown";
    }
}

// Функция для преобразования строки в Action
[[nodiscard]] Action string_to_action(const std::string &str) {
    if (str == "decrypt") {
        return Action::DECRYPT;
    }
    if (str == "status") {
        return Action::STATUS;
    }
    if (str == "ping") {
        return Action::PING;
    }
    if (str == "quit") {
        return Action::QUIT;
    }
    return Action::UNKNOWN;
}

// Функция для преобразования MessageType в строку
[[nodiscard]] std::string type_to_string(MessageType type) {
    switch (type) {
        case MessageType::REQUEST:
            return "request";
        case MessageType::RESPONSE:
            return "response";
        case MessageType::ERROR:
            return "error";
        default:
            return "unknown";
    }
}

// Функция для преобразования строки в MessageType
[[nodiscard]] MessageType string_to_type(const std::string &str) {
    if (str == "request") {
        return MessageType::REQUEST;
    }
    if (str == "response") {
        return MessageType::RESPONSE;
    }
    if (str == "error") {
        return MessageType::ERROR;
    }
    return MessageType::UNKNOWN;
}

[[nodiscard]] std::string cipher_to_string(decrypt::CipherType type) {
    switch (type) {
        case decrypt::CipherType::CAESAR:
            return "caesar";
        case decrypt::CipherType::VIGENERE:
            return "vigenere";
        case decrypt::CipherType::XOR:
            return "XOR";
        default:
            return "unknown";
    }
}

[[nodiscard]] decrypt::CipherType string_to_cipher(const std::string &str) {
    if (str == "caesar") {
        return decrypt::CipherType::CAESAR;
    }
    if (str == "vigenere") {
        return decrypt::CipherType::VIGENERE;
    }
    if (str == "XOR") {
        return decrypt::CipherType::XOR;
    }
    return decrypt::CipherType::UNKNOWN;
}

[[nodiscard]] json DecryptPayload::to_json() const {
    json j = {
        {"chipher", cipher_to_string(cipher)},
        {"cipher_text", cipher_text},
        {"start_key", start_key},
        {"end_key", end_key}};
    return j;
}

[[nodiscard]] std::unique_ptr<Payload> DecryptPayload::from_json(const json &j
) const {
    DecryptPayload payload;
    payload.cipher = string_to_cipher(j["chipher"]);
    payload.cipher_text = j["cipher_text"];
    payload.start_key = j["start_key"];
    payload.end_key = j["end_key"];
    return std::make_unique<DecryptPayload>(std::move(payload));
}

[[nodiscard]] json StatusPayload::to_json() const {
    json j = {
        {"chipher", cipher_to_string(cipher)},
        {"cipher_text", cipher_text},
        {"progress", progress}};
    return j;
}

[[nodiscard]] std::unique_ptr<Payload> StatusPayload::from_json(const json &j
) const {
    StatusPayload payload;
    payload.cipher = string_to_cipher(j["chipher"]);
    payload.cipher_text = j["cipher_text"];
    payload.progress = j["progress"];
    return std::make_unique<StatusPayload>(std::move(payload));
}

[[nodiscard]] json PingPayload::to_json() const {
    json j = {{"code", code}};
    return j;
}

[[nodiscard]] std::unique_ptr<Payload> PingPayload::from_json(const json &j
) const {
    PingPayload payload;
    payload.code = j["code"];
    return std::make_unique<PingPayload>(std::move(payload));
}

[[nodiscard]] json QuitPayload::to_json() const {
    json j = {{"code", code}};
    return j;
}

[[nodiscard]] std::unique_ptr<Payload> QuitPayload::from_json(const json &j
) const {
    QuitPayload payload;
    payload.code = j["code"];
    return std::make_unique<QuitPayload>(std::move(payload));
}

[[nodiscard]] json Message::to_json() const {
    json j = {
        {"type", type_to_string(type)},
        {"action", action_to_string(action)},
        {"payload", payload->to_json()}};
    return j;
}

Message Message::from_json(const json &j) {
    Message msg;
    msg.type = string_to_type(j.value("type", ""));
    msg.action = string_to_action(j.value("action", ""));
    switch (msg.action) {
        case Action::DECRYPT:
            msg.payload = DecryptPayload().from_json(j["payload"]);
            break;
        case Action::STATUS:
            msg.payload = StatusPayload().from_json(j["payload"]);
            break;
        case Action::PING:
            msg.payload = PingPayload().from_json(j["payload"]);
            break;
        case Action::QUIT:
            msg.payload = QuitPayload().from_json(j["payload"]);
            break;
        case Action::UNKNOWN:
            throw;
    }
    return msg;
}

Message Message::create_decrypt_request(std::unique_ptr<Payload> payload) {
    return {MessageType::REQUEST, Action::DECRYPT, std::move(payload)};
}

Message Message::create_decrypt_response(std::unique_ptr<Payload> payload) {
    return {MessageType::RESPONSE, Action::DECRYPT, std::move(payload)};
}

Message Message::create_status_request(std::unique_ptr<Payload> payload) {
    return {MessageType::REQUEST, Action::STATUS, std::move(payload)};
}

Message Message::create_status_response(std::unique_ptr<Payload> payload) {
    return {MessageType::RESPONSE, Action::STATUS, std::move(payload)};
}

Message Message::create_ping_request(std::unique_ptr<Payload> payload) {
    return {MessageType::REQUEST, Action::PING, std::move(payload)};
}

Message Message::create_pong_response(std::unique_ptr<Payload> payload) {
    return {MessageType::RESPONSE, Action::PING, std::move(payload)};
}

asio::awaitable<void> Connection::send_message(const Message &msg) {
    std::string json_str = msg.to_json().dump();

    // Формат: [HEADER][JSON]
    uint32_t size = json_str.size();

    // Отправляем размер
    co_await asio::async_write(
        socket_, asio::buffer(&size, sizeof(size)), asio::use_awaitable
    );

    // Отправляем JSON
    co_await asio::async_write(
        socket_, asio::buffer(json_str), asio::use_awaitable
    );
}

asio::awaitable<Message> Connection::read_message() {
    uint32_t size{};
    co_await asio::async_read(
        socket_, asio::buffer(&size, sizeof(size)), asio::use_awaitable
    );

    if (size > MAX_MESSAGE_SIZE) {
        throw std::runtime_error("Message too large");
    }

    // Читаем JSON
    std::vector<char> buffer(size);
    co_await asio::async_read(
        socket_, asio::buffer(buffer), asio::use_awaitable
    );

    // Парсим JSON
    json j = json::parse(std::string(buffer.begin(), buffer.end()));

    co_return Message::from_json(j);
}
}  // namespace json_protocol
