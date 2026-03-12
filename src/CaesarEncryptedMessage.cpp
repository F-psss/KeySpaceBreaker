#include <CaesarEncryptedMessage.hpp>

namespace server {

std::string CaesarEncryptedMessage::get_text() const {
    return m_encrypted_text;
}

std::vector<int> CaesarEncryptedMessage::generate_key_space() const {
    std::vector<int> keys(26);
    for (int i = 0; i < 26; ++i) {
        keys[i] = i;
    }
    return keys;
}

std::string CaesarEncryptedMessage::decrypt(int key) const {
    std::string decrypted_text;
    for (char c : m_encrypted_text) {
        if (isalpha(c) != 0) {
            char offset = (islower(c) != 0) ? 'a' : 'A';
            decrypted_text += (c - offset - key + 26) % 26 + offset;
        } else {
            decrypted_text += c;
        }
    }
    return decrypted_text;
}

}  // namespace server
