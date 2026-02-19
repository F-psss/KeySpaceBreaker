#ifndef STATIC_POLICY_HPP
#define STATIC_POLICY_HPP

#include "Policy.hpp"

namespace Server {

class StaticPolicy : public Policy {
public:
    StaticPolicy(int total_keys, int unit_size);
    std::shared_ptr<Unit> get_next_unit() override;

private:
    int m_total_keys;
    int m_unit_size;
    int m_current_start;
};

} // namespace Server

#endif // STATIC_POLICY_HPP
