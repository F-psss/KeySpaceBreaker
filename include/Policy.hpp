#ifndef POLICY_HPP
#define POLICY_HPP

#include <memory>
#include "Unit.hpp"

namespace server {

class Policy {
public:
    virtual ~Policy() = default;

    virtual Unit get_next_unit() = 0;
};

}  // namespace server

#endif  // POLICY_HPP
