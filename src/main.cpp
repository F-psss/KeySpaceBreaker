#include "CaesarEncryptedMessage.hpp"
#include "Coordinator.hpp"
#include "Worker.hpp"
#include "StaticPolicy.hpp"
#include <iostream>

int main() {
    try {
        // чтение текста из файла
        std::string encrypted_text = Server::EncryptedMessage::read_from_file("text.txt");

        // создание объекта зашифрованного текста. тип: цезарь
        auto message = std::make_shared<Server::CaesarEncryptedMessage>(encrypted_text);

        // генерация пространства ключей
        auto key_space = message->generate_key_space();

        // создание политики для разбиения ключей
        auto policy = std::make_shared<Server::StaticPolicy>(26, 3);  // 26 ключей для цезаря, разбиваем на юниты по 3 ключа

        // создание координатора, в конструкторе происходит разбиение на юниты
        Server::Coordinator coordinator(message, policy);

        // назначение юнитов воркерам
        for (int i = 0; i < 3; ++i) {
            coordinator.assign_to_worker(std::make_shared<Server::CaesarWorker>());
        }

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    return 0;
}
