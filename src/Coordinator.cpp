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
      m_best_result{"-1", std::numeric_limits<double>::max(), ""} {
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
        if (m_solved) {
            co_return;
        }
        if (m_units[i].get_status() == UnitStatus::Unassigned) {
            m_units[i].mark_as_leased();
            m_pending_units[i] = {worker, std::chrono::steady_clock::now()};
            co_await worker->send_unit(i);
            co_return;
        }
    }
    co_return;
}

void Coordinator::cand_to_best(const Result &cand) {
    if (cand.score_ < m_best_result.score_) {
        m_best_result = cand;
        if (m_mode == decrypt::VigenereMode::FAST &&
            m_best_result.score_ < FAST_THRESHOLD) {
            m_solved = true;
        }
    }
}

bool Coordinator::mark_unit_done(std::size_t index) {
    if (m_units[index].get_status() == UnitStatus::Leased) {
        m_units[index].mark_as_done();
        // m_pending_units.erase
        m_done_count++;
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
    if (m_solved) {
        return true;
    }
    for (const auto &unit : m_units) {
        if (unit.get_status() != UnitStatus::Done) {
            return false;
        }
    }
    return true;
}

void Coordinator::save_checkpoint(const std::string &path) const {
    nlohmann::json j;
    // TODO: мб стоит поменять кэш текста, но пока такой:
    j["task_id"] = std::to_string(m_message->get_text().size()) + "_" +
                   m_message->get_text().substr(0, 32);
    j["text"] = m_message->get_text();
    j["best_result"] = m_best_result.to_json();
    j["units"] = nlohmann::json::array();
    for (const auto &u : m_units) {
        j["units"].push_back(u.to_json());
    }

    // Атомарная запись через временный файл
    std::string tmp_path = path + ".tmp";
    {
        std::ofstream out(tmp_path);
        if (!out.is_open()) {
            std::cerr << "Failed to open checkpoint tmp file: " << tmp_path
                      << std::endl;
            return;
        }
        out << j.dump(2);
    }
    std::rename(tmp_path.c_str(), path.c_str());
}

bool Coordinator::load_checkpoint(
    const std::string &path,
    const std::string &current_text
) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        in >> j;

        std::string task_id = std::to_string(current_text.size()) + "_" +
                              current_text.substr(0, 32);
        if (j["task_id"] != task_id) {
            std::cout << "Checkpoint is for different task, ignoring\n";
            return false;
        }

        m_best_result = Result::from_json(j["best_result"]);
        m_units.clear();
        for (const auto &uj : j["units"]) {
            m_units.push_back(Unit::from_json(uj));
        }
        m_pending_units.clear();
        m_done_count = 0;
        for (const auto &u : m_units) {
            if (u.get_status() == UnitStatus::Done) {
                m_done_count++;
            }
        }
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to load checkpoint: " << e.what() << std::endl;
        return false;
    }
}
}  // namespace server
