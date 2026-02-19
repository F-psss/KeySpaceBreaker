#include <JSON_Protocol.hpp>

using json = nlohmann::json;

[[nodiscard]] json JsonProtocol::Message::to_json() const {
    json j = {{"type", type}};

    if (!action.empty()) {
        j["action"] = action;
    }
    if (!payload.empty()) {
        j["payload"] = payload;
    }
    if (!metadata.empty()) {
        j["metadata"] = metadata;
    }
    if (status != "success") {
        j["status"] = status;
    }

    return j;
}

JsonProtocol::Message JsonProtocol::Message::from_json(const json &j) {
    Message msg;
    msg.type = j.value("type", "");
    msg.action = j.value("action", "");

    if (j.contains("payload")) {
        msg.payload = j["payload"];
    }
    if (j.contains("metadata")) {
        msg.metadata = j["metadata"];
    }
    msg.status = j.value("status", "success");

    return msg;
}

JsonProtocol::Message JsonProtocol::Message::create_request(
    const std::string &action,
    const json &payload,
    const json &metadata
) {
    Message msg;
    msg.type = "request";
    msg.action = action;
    msg.payload = payload;
    msg.metadata = metadata;
    return msg;
}

JsonProtocol::Message JsonProtocol::Message::create_response(
    const std::string &action,
    const json &payload,
    const std::string &status
) {
    Message msg;
    msg.type = "response";
    msg.action = action;
    msg.payload = payload;
    msg.status = status;
    return msg;
}

asio::awaitable<void> JsonProtocol::Connection::send_message(
    const JsonProtocol::Message &msg
) {
    std::string json_str = msg.to_json().dump();

    // Формат: [HEADER][JSON]
    uint32_t size = htonl(json_str.size());

    // Отправляем размер
    co_await asio::async_write(
        socket_, asio::buffer(&size, sizeof(size)), asio::use_awaitable
    );

    // Отправляем JSON
    co_await asio::async_write(
        socket_, asio::buffer(json_str), asio::use_awaitable
    );
}

asio::awaitable<JsonProtocol::Message> JsonProtocol::Connection::read_message(
) {
    uint32_t size{};
    co_await asio::async_read(
        socket_, asio::buffer(&size, sizeof(size)), asio::use_awaitable
    );

    size = ntohl(size);

    if (size > JsonProtocol::MAX_MESSAGE_SIZE) {
        throw std::runtime_error("Message too large");
    }

    // Читаем JSON
    std::vector<char> buffer(size);
    co_await asio::async_read(
        socket_, asio::buffer(buffer), asio::use_awaitable
    );

    // Парсим JSON
    json j = json::parse(std::string(buffer.begin(), buffer.end()));

    co_return JsonProtocol::Message::from_json(j);
}
