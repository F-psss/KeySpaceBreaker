#ifndef WORKER_HPP
#define WORKER_HPP

#include <memory>
#include "Unit.hpp"

namespace Server {

class Worker {
public:
    virtual ~Worker() = default;
    virtual void process_unit(std::shared_ptr<Unit> unit) = 0;
};

class CaesarWorker : public Worker {
public:
    void process_unit(std::shared_ptr<Unit> unit) override;
};


} // namespace Server

#endif // WORKER_HPP
