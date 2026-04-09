#include "StaticPolicy.hpp"
#include <iostream>
#include "Unit.hpp"

namespace server {

StaticPolicy::StaticPolicy(int total_keys, int unit_size)
    : m_total_keys(total_keys), m_unit_size(unit_size), m_current_start(0) {
}

[[nodiscard]] Unit StaticPolicy::get_next_unit() {
    if (m_current_start >= m_total_keys) {
        return {-1, -1};
    }

    int end = std::min(m_current_start + m_unit_size, m_total_keys);
    Unit unit(m_current_start, end);
    m_current_start = end;

    std::cout << "Создание юнита: Диапазон [" << unit.get_start() << ", "
              << unit.get_end() << "]" << std::endl;

    return unit;
}

}  // namespace server
