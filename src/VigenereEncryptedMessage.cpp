#include <VigenereEncryptedMessage.hpp>

namespace server {
std::string VigenereEncryptedMessage::get_text() const {
    return m_encrypted_text;
}
std::string VigenereEncryptedMessage::decrypt(const std::string &key) const {
    std::string result;
    size_t key_len = key.length();
    if (key_len == 0) {
        return m_encrypted_text;
    }
    size_t key_index = 0;
    for (char c : m_encrypted_text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            char offset = std::islower(c) ? 'a' : 'A';
            int shift = std::tolower(key[key_index % key_len]) - 'a';
            result += (c - offset - shift + 26) % 26 + offset;
            ++key_index;
        } else {
            result += c;
        }
    }
    return result;
}
std::vector<int> VigenereEncryptedMessage::generate_key_space() const {
    return {};
}
}  // namespace server

// TODO: get_text()