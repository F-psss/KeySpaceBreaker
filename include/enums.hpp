#pragma onse

namespace json_protocol {
enum class MessageType { REQUEST, RESPONSE, ERROR, UNKNOWN };
enum class Action { DECRYPT, STATUS, PING, QUIT, UNKNOWN };
}  // namespace json_protocol

namespace decrypt {
enum class CipherType { CAESAR, VIGENERE, XOR, UNKNOWN };
}
