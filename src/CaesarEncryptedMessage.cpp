#include "CaesarEncryptedMessage.hpp"

namespace Server {


CaesarEncryptedMessage::CaesarEncryptedMessage(const std::string& encrypted_text)
    : m_encrypted_text(encrypted_text) {}


std::string CaesarEncryptedMessage::get_text() const {
    return m_encrypted_text;
}


std::vector<int> CaesarEncryptedMessage::generate_key_space() const {
    std::vector<int> keys;
    for (int i = 0; i < 26; ++i) {
        keys.push_back(i);
    }
    return keys;
}

std::string CaesarEncryptedMessage::decrypt(int key) const {
    std::string decrypted_text;
    for (char c : m_encrypted_text) {
        if (isalpha(c)) {
            char offset = islower(c) ? 'a' : 'A';
            decrypted_text += (c - offset - key + 26) % 26 + offset;
        } else {
            decrypted_text += c;
        }
    }
    return decrypted_text;
}

} // namespace Server
