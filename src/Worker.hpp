#ifndef WORKER_HPP
#define WORKER_HPP

#include <memory>
#include "Unit.hpp"
#include "EncryptedMessage.hpp"
#include "CaesarEncryptedMessage.hpp"

namespace Server {

class Worker {
public:
    virtual ~Worker() = default;
    virtual void process_unit(std::shared_ptr<Unit> unit) = 0;
};

class CaesarWorker : public Worker {
public:
    explicit CaesarWorker(std::shared_ptr<const EncryptedMessage> message);
    void process_unit(std::shared_ptr<Unit> unit) override;
    struct Result
    {
        int key_;
        double score_;
        std::string text_;
    };
    
    Result get_best_result() const {return m_best_result;}
private:
    std::shared_ptr<const EncryptedMessage> m_message;
    Result m_best_result;
    double calculate_score(const std::string& text);
    static const double ENGLISH_FREQS[26];
    

};


} // namespace Server

#endif // WORKER_HPP
