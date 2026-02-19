#ifndef CAESAR_ENCRYPTED_MESSAGE_HPP
#define CAESAR_ENCRYPTED_MESSAGE_HPP

#include "EncryptedMessage.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Server {

class CaesarEncryptedMessage : public EncryptedMessage {
public:

    CaesarEncryptedMessage(const std::string& encrypted_text);

    
    std::string get_text() const override;

    
    std::vector<int> generate_key_space() const override;

    
    std::string decrypt(int key) const override;

private:
    std::string m_encrypted_text;
};

} // namespace Server

#endif // CAESAR_ENCRYPTED_MESSAGE_HPP
