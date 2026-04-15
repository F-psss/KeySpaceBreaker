#ifndef UNIT_HPP
#define UNIT_HPP

#include <string>

namespace server {

enum class UnitStatus { Unassigned, Leased, Done };

struct Result {
    int key_;
    double score_;
    std::string text_{};
};

class Unit {
public:
    Unit(int start, int end) : m_start(start), m_end(end) {
    }

    [[nodiscard]] int get_start() const {
        return m_start;
    }

    [[nodiscard]] int get_end() const {
        return m_end;
    }

    [[nodiscard]] UnitStatus get_status() const {
        return m_status;
    }

    [[nodiscard]] bool is_end() const {
        return (m_start == -1) && (m_end == -1);
    }

    void mark_as_leased() {
        m_status = UnitStatus::Leased;
    }

    void mark_as_unassigned() {
        m_status = UnitStatus::Unassigned;
    }

    void mark_as_done() {
        m_status = UnitStatus::Done;
    }

private:
    int m_start;
    int m_end;
    UnitStatus m_status = UnitStatus::Unassigned;
};

}  // namespace server

#endif  // UNIT_HPP
