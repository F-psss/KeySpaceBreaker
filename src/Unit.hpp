#ifndef UNIT_HPP
#define UNIT_HPP

namespace Server {

enum class UnitStatus {
    Unassigned,
    Leased,
    Done
};

class Unit {
public:
    Unit(int start, int end) 
        : m_start(start), m_end(end), m_status(UnitStatus::Unassigned) {}

    int get_start() const { return m_start; }
    int get_end() const { return m_end; }
    UnitStatus get_status() const { return m_status; }

    void mark_as_leased() { m_status = UnitStatus::Leased; }
    void mark_as_done() { m_status = UnitStatus::Done; }

private:
    int m_start;
    int m_end;
    UnitStatus m_status;
};

} // namespace Server

#endif // UNIT_HPP
