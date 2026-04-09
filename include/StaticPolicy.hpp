#ifndef STATIC_POLICY_HPP
#define STATIC_POLICY_HPP

#include "Policy.hpp"

namespace server {

class StaticPolicy : public Policy {
public:
    StaticPolicy(int total_keys, int unit_size);
    Unit get_next_unit() override;

private:
    int m_total_keys;
    int m_unit_size;
    int m_current_start;
};

}  // namespace server

#endif  // STATIC_POLICY_HPP
