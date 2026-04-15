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
    for (int i = 0; i < m_units.size(); i++) {
        if (m_units[i].get_status() == UnitStatus::Unassigned) {
            m_units[i].mark_as_leased();
            m_pending_units[i] = {worker, std::chrono::steady_clock::now()};
            std::cout << "Add pending_unit\n";
            co_await worker->send_unit(i);
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

bool Coordinator::mark_unit_done(std::size_t index) {
    if (m_units[index].get_status() == UnitStatus::Leased) {
        m_units[index].mark_as_done();
        // m_pending_units.erase
        auto it = m_pending_units.find(index);
        if (it != m_pending_units.end()) {
            m_pending_units.erase(it);
        }
        return true;
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
