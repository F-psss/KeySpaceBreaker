#ifndef ENCRYPTED_MESSAGE_HPP
#define ENCRYPTED_MESSAGE_HPP

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace server {

class EncryptedMessage {
public:
    EncryptedMessage(const EncryptedMessage &) = delete;
    EncryptedMessage &operator=(const EncryptedMessage &) = delete;
    EncryptedMessage(EncryptedMessage &&) = default;
    EncryptedMessage &operator=(EncryptedMessage &&) = default;
    virtual ~EncryptedMessage() = default;

    [[nodiscard]] virtual std::string get_text() const = 0;
    [[nodiscard]] virtual std::vector<int> generate_key_space() const = 0;
    [[nodiscard]] virtual std::string decrypt(int key) const = 0;

    static std::string read_from_file(const std::string &file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл.");
        }
        return {
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()};
    }

protected:
    EncryptedMessage() = default;
};

}  // namespace server

#endif  // ENCRYPTED_MESSAGE_HPP
