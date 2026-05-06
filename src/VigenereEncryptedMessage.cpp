#include <VigenereEncryptedMessage.hpp>
namespace server {
std::string decrypt_vigenere(const std::string& text, const std::string& key) {
    std::string result;
    int key_len = key.size();

    for (size_t i = 0, j = 0; i < text.size(); ++i) {
        char c = text[i];

        if (std::isalpha(c)) {
            char base = std::islower(c) ? 'a' : 'A';
            int shift = std::tolower(key[j % key_len]) - 'a';
            result += (c - base - shift + 26) % 26 + base;
            j++;
        } else {
            result += c;
        }
    }

    return result;
}
}// namespace server
// TODO: get_text(), descrypt()