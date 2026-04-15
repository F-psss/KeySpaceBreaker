#pragma once

#include <WorkerSession.hpp>
#include <cstddef>
#include <memory>
#include <vector>
#include "Decryptor.hpp"
#include "EncryptedMessage.hpp"
#include "NetworkWorker.hpp"
#include "Policy.hpp"
#include "Unit.hpp"

namespace server {

const std::chrono::seconds UNIT_TIMEOUT(3);

class Coordinator {
public:
    Coordinator(
        std::shared_ptr<EncryptedMessage> message,
        std::shared_ptr<Policy> policy
    );
    asio::awaitable<void> assign_to_worker(std::shared_ptr<WorkerSession> worker
    );

    size_t unit_count() {
        return m_units.size();
    }

    [[nodiscard]] std::shared_ptr<EncryptedMessage> get_message() const {
        return m_message;
    };

    void cand_to_best(const Result &cand);

    bool mark_unit_done(std::size_t index);

    [[nodiscard]] Unit &get_unit(std::size_t index) {
        return m_units.at(index);
    }

    void mark_as_unassigned_by_index(std::size_t index) {
        m_units[index].mark_as_unassigned();
    }

    [[nodiscard]] bool has_unassigned_units() const;

    [[nodiscard]] bool all_units_done() const;

    [[nodiscard]] const Result &best_result() const {
        return m_best_result;
    };

private:
    // constructor
    std::shared_ptr<EncryptedMessage> m_message;
    std::shared_ptr<Policy> m_policy;
    Result m_best_result;

    // gen
    std::vector<Unit> m_units{};

    struct PendingUnit {
        std::shared_ptr<WorkerSession> worker;
        std::chrono::steady_clock::time_point start_time;
    };

public:
    std::unordered_map<size_t, PendingUnit> m_pending_units;
};

}  // namespace server

// COORDINATOR_HPP
