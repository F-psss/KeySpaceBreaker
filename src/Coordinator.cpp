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

            break;
        }
    }
    std::cout << "\n\n\n\n\n\n\nBEST\n"
              << m_best_result.key_ << '\n'
              << m_best_result.score_ << '\n'
              << m_best_result.text_;
}

void Coordinator::cand_to_best(const Result &cand) {
    if (cand.score_ < m_best_result.score_) {
        m_best_result = cand;
    }
}

}  // namespace server
