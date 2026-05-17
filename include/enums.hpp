#pragma once

namespace json_protocol {
enum class MessageType { REQUEST, RESPONSE, ERROR, UNKNOWN };
enum class Action {
    DECRYPT,
    STATUS,
    PING,
    QUIT,
    PEER_HELLO,
    PEER_PING,         // heartbeat from primary to backup
    PEER_ELECTION,     // starting the election
    PEER_ALIVE,        // i'm alive
    PEER_COORDINATOR,  // i'm the leader now
    PEER_CHECKPOINT,   // checkpoint replication
    UNKNOWN

};
}  // namespace json_protocol

namespace decrypt {
enum class CipherType { CAESAR, VIGENERE, XOR, UNKNOWN };
enum class VigenereMode { BRUTE, FAST, UNKNOWN };
}  // namespace decrypt
