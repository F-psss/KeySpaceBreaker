#include "Coordinator.hpp"
#include "Worker.hpp"
#include <iostream>

namespace Server {

Coordinator::Coordinator(std::shared_ptr<EncryptedMessage> message, std::shared_ptr<Policy> policy)
    : m_message(message), m_policy(policy) {
    
    // Генерация юнитов на основе диапазонов, предоставленных политикой
    while (true) {
        auto unit = m_policy->get_next_unit();
        if (!unit) break;
        m_units.push_back(unit);
    }
}

void Coordinator::assign_to_worker(std::shared_ptr<Worker> worker) {
    for (auto& unit : m_units) {
        if (unit->get_status() == UnitStatus::Unassigned) {

            unit->mark_as_leased();
            
            // отладочная информация
            std::cout << "Назначен юнит " << ": "
                      << "Диапазон [" << unit->get_start() << ", " << unit->get_end() << "]"
                      << ", Статус: " << (unit->get_status() == Server::UnitStatus::Unassigned ? "Unassigned" : (unit->get_status() == Server::UnitStatus::Leased ? "Leased" : "Done"))
                      << ", Размер: " << (unit->get_end() - unit->get_start()) << " ключей." << std::endl;

            
            // worker->process_unit(unit);

            break;
        }
    }
}

} // namespace Server
