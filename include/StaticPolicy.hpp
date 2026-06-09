#ifndef STATIC_POLICY_HPP
#define STATIC_POLICY_HPP

#include "Policy.hpp"

namespace server {

class StaticPolicy : public Policy {
public:
    StaticPolicy(
        int total_keys,
        int unit_size,
        double noise,
        decrypt::CipherType m_cipher,
        decrypt::VigenereMode m_mode,
        int key_length
    );
    Unit get_next_unit() override;

private:
    int m_total_keys;
    int m_unit_size;
    int m_current_start;
    double m_noise;
    decrypt::CipherType m_cipher;
    decrypt::VigenereMode m_mode;
    int m_key_length;
};

}  // namespace server

#endif  // STATIC_POLICY_HPP
