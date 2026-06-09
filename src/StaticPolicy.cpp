#include "StaticPolicy.hpp"
#include <iostream>
#include "Unit.hpp"

namespace server {

StaticPolicy::StaticPolicy(
    int total_keys,
    int unit_size,
    double noise,
    decrypt::CipherType cipher,
    decrypt::VigenereMode mode,
    int key_length
)
    : m_total_keys(total_keys),
      m_unit_size(unit_size),
      m_current_start(0),
      m_noise(noise),
      m_cipher(cipher),
      m_mode(mode),
      m_key_length(key_length) {
}

Unit StaticPolicy::get_next_unit() {
    if (m_current_start >= m_total_keys) {
        return {-1, -1, m_cipher, m_mode, m_key_length, 0.0};
    }
    int end = std::min(m_current_start + m_unit_size, m_total_keys) - 1;
    Unit unit(m_current_start, end, m_cipher, m_mode, m_key_length, m_noise);
    m_current_start = end + 1;
    return unit;
}
}  // namespace server
