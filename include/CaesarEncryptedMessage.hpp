#ifndef CAESAR_ENCRYPTED_MESSAGE_HPP
#define CAESAR_ENCRYPTED_MESSAGE_HPP

#include <EncryptedMessage.hpp>
#include <memory>
#include <string>
#include <vector>

namespace server {

class CaesarEncryptedMessage : public EncryptedMessage {
public:
    CaesarEncryptedMessage(std::string encrypted_text)
        : m_encrypted_text(std::move(encrypted_text)) {
    }

    [[nodiscard]] std::string get_text() const override;
    [[nodiscard]] std::vector<int> generate_key_space() const override;
    [[nodiscard]] std::string decrypt(int key) const override;

private:
    std::string m_encrypted_text;
};

}  // namespace server

#endif  // CAESAR_ENCRYPTED_MESSAGE_HPP
