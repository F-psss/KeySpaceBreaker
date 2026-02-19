#ifndef ENCRYPTED_MESSAGE_HPP
#define ENCRYPTED_MESSAGE_HPP

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

namespace Server {


class EncryptedMessage {
public:
    virtual ~EncryptedMessage() = default;

    virtual std::string get_text() const = 0;

    virtual std::vector<int> generate_key_space() const = 0;

    virtual std::string decrypt(int key) const = 0;

    static std::string read_from_file(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл.");
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
};

} // namespace Server

#endif // ENCRYPTED_MESSAGE_HPP
