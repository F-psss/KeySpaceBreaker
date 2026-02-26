#include "CaesarEncryptedMessage.hpp"
#include "Coordinator.hpp"
#include "Worker.hpp"
#include "StaticPolicy.hpp"
#include <iostream>

int main() {
    try {
        std::string encrypted_text = Server::EncryptedMessage::read_from_file("text.txt");

        auto message = std::make_shared<Server::CaesarEncryptedMessage>(encrypted_text);

         Server::CaesarWorker worker(message);

        std::vector<std::shared_ptr<Server::Unit>> units;
        units.push_back(std::make_shared<Server::Unit>(0, 12));
        units.push_back(std::make_shared<Server::Unit>(13, 25));

        for (auto& unit : units) {
            unit->mark_as_leased();
            worker.process_unit(unit);
        }

        auto result = worker.get_best_result();
        std::cout << "=== RESULT ===\n" << std::endl;
        std::cout << "Best Key found: " << result.key_ << std::endl;
        std::cout << "Score: " << result.score_ << std::endl;
        std::cout << "Decrypted Text: " << result.text_ << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}