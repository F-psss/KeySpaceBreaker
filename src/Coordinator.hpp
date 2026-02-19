#ifndef COORDINATOR_HPP
#define COORDINATOR_HPP

#include <vector>
#include <memory>
#include "EncryptedMessage.hpp"
#include "Worker.hpp"
#include "Policy.hpp"
#include "Unit.hpp"

namespace Server {

class Coordinator {
public:
    Coordinator(std::shared_ptr<EncryptedMessage> message, std::shared_ptr<Policy> policy);
    void assign_work_units();
    void assign_to_worker(std::shared_ptr<Worker> worker);

private:
    std::shared_ptr<EncryptedMessage> m_message;
    std::vector<std::shared_ptr<Unit>> m_units; 
    std::vector<std::shared_ptr<Worker>> m_workers;
    std::shared_ptr<Policy> m_policy;
};

} // namespace Server

#endif // COORDINATOR_HPP
