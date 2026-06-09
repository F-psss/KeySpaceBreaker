#pragma once
#include <EncryptedMessage.hpp>
#include <string>
#include <vector>

namespace server {

class VigenereEncryptedMessage : public EncryptedMessage {
public:
    VigenereEncryptedMessage(std::string encrypted_text)
        : m_encrypted_text(std::move(encrypted_text)) {
    }

    [[nodiscard]] std::string get_text() const override;
    [[nodiscard]] std::vector<int> generate_key_space() const override;
    [[nodiscard]] std::string decrypt(const std::string &key) const override;

private:
    std::string m_encrypted_text;
};

}  // namespace server
