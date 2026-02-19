#ifndef POLICY_HPP
#define POLICY_HPP

#include "Unit.hpp"
#include <memory>

namespace Server {

class Policy {
public:
    virtual ~Policy() = default;

    virtual std::shared_ptr<Unit> get_next_unit() = 0;
};

} // namespace Server

#endif // POLICY_HPP
