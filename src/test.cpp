#include <iostream>
#include "CaesarEncryptedMessage.hpp"
#include "Coordinator.hpp"
#include "Decryptor.hpp"
#include "StaticPolicy.hpp"

int main() {
    try {
        char buffer[256];
        std::cout << "Current dir: " << getcwd(buffer, sizeof(buffer))
                  << std::endl;
        std::string encrypted_text =
            server::EncryptedMessage::read_from_file("src/text.txt");

        auto message =
            std::make_shared<server::CaesarEncryptedMessage>(encrypted_text);

        server::CaesarDecryptor worker(message);

        std::vector<std::shared_ptr<server::Unit>> units;
        units.push_back(std::make_shared<server::Unit>(0, 12));
        units.push_back(std::make_shared<server::Unit>(13, 25));

        for (auto &unit : units) {
            unit->mark_as_leased();
            worker.process_unit(unit);
        }

        auto result = worker.get_best_result();
        std::cout << "=== RESULT ===\n" << std::endl;
        std::cout << "Best Key found: " << result.key_ << std::endl;
        std::cout << "Score: " << result.score_ << std::endl;
        std::cout << "Decrypted Text: " << result.text_ << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
