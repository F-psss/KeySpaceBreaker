#include <Coordinator.hpp>
#include <iostream>
#include "Unit.hpp"

namespace server {

Coordinator::Coordinator(
    std::shared_ptr<EncryptedMessage> message,
    std::shared_ptr<Policy> policy
)
    : m_message(message),
      m_policy(policy),
      m_best_result{-1, std::numeric_limits<double>::max(), ""} {
    // Генерация юнитов на основе диапазонов, предоставленных политикой
    while (true) {
        auto unit = m_policy->get_next_unit();
        if (unit.is_end()) {
            break;
        }
        m_units.push_back(unit);
    }
}

asio::awaitable<void> Coordinator::assign_to_worker(
    std::shared_ptr<WorkerSession> worker
) {
    for (auto &unit : m_units) {
        if (unit.get_status() == UnitStatus::Unassigned) {
            unit.mark_as_leased();

            // отладочная информация
            std::cout << "Назначен юнит "
                      << ": "
                      << "Диапазон [" << unit.get_start() << ", "
                      << unit.get_end() << "]"
                      << ", Статус: "
                      << (unit.get_status() == server::UnitStatus::Unassigned
                              ? "Unassigned"
                              : (unit.get_status() == server::UnitStatus::Leased
                                     ? "Leased"
                                     : "Done"))
                      << ", Размер: " << (unit.get_end() - unit.get_start())
                      << " ключей." << std::endl;

            co_await worker->send_unit(unit);
            co_return;
        }
    }
    co_return;
}


void Coordinator::cand_to_best(const Result &cand) {
    if (cand.score_ < m_best_result.score_) {
        m_best_result = cand;
    }
}

bool Coordinator::mark_one_leased_unit_done() {
    for (auto &unit : m_units) {
        if (unit.get_status() == UnitStatus::Leased) {
            unit.mark_as_done();
            return true;
        }
    }
    return false;
}

bool Coordinator::has_unassigned_units() const {
    for (const auto &unit : m_units) {
        if (unit.get_status() == UnitStatus::Unassigned) {
            return true;
        }
    }
    return false;
}

bool Coordinator::all_units_done() const {
    for (const auto &unit : m_units) {
        if (unit.get_status() != UnitStatus::Done) {
            return false;
        }
    }
    return true;
}

}  // namespace server
