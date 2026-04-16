#include "StaticPolicy.hpp"
#include <iostream>
#include "Unit.hpp"

namespace server {

StaticPolicy::StaticPolicy(int total_keys, int unit_size, double noise)
    : m_total_keys(total_keys), m_unit_size(unit_size), m_current_start(0), m_noise(noise) {
}

[[nodiscard]] Unit StaticPolicy::get_next_unit() {
    if (m_current_start >= m_total_keys) {
        return {-1, -1};
    }

    int end = std::min(m_current_start + m_unit_size, m_total_keys) - 1;
    std::cout << "######################" << m_noise << "###################" << std::endl;
    Unit unit(m_current_start, end, m_noise);
    m_current_start = end + 1;

    std::cout << "Создание юнита: Диапазон [" << unit.get_start() << ", "
              << unit.get_end() << "]" << std::endl;

    return unit;
}

}  // namespace server
