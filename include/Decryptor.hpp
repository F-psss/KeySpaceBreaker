#ifndef WORKER_HPP
#define WORKER_HPP

#include <CaesarEncryptedMessage.hpp>
#include <EncryptedMessage.hpp>
#include <Dictionary.hpp>
#include <Unit.hpp>
#include <limits>
#include <memory>

namespace server {

class Decryptor {
public:
    Decryptor(const Decryptor &) = delete;
    Decryptor &operator=(const Decryptor &) = delete;
    Decryptor(Decryptor &&) = default;
    Decryptor &operator=(Decryptor &&) = default;
    virtual ~Decryptor() = default;
    virtual void process_unit(std::shared_ptr<Unit> unit) = 0;

protected:
    Decryptor() = default;
};

class CaesarDecryptor : public Decryptor {
public:
    explicit CaesarDecryptor(std::shared_ptr<const EncryptedMessage> message)
        : m_message(std::move(message)),
          m_best_result{-1, std::numeric_limits<double>::max(), ""} {
        m_dict.load("../words.txt");
    }

    void process_unit(std::shared_ptr<Unit> unit) override;

    [[nodiscard]] Result get_best_result() const {
        return m_best_result;
    }

private:
    std::shared_ptr<const EncryptedMessage> m_message;
    double m_noise;
    Dictionary m_dict;
    Result m_best_result;
    static double calculate_score(const std::string &text);
    static const double ENGLISH_FREQS[26];
};

}  // namespace server

#endif  // WORKER_HPP
