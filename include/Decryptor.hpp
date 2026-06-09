#ifndef WORKER_HPP
#define WORKER_HPP

#include <Dictionary.hpp>
#include <EncryptedMessage.hpp>
#include <Unit.hpp>
#include <limits>
#include <memory>
#include <unordered_map>
namespace server {

class Decryptor {
public:
    Decryptor(const Decryptor &) = delete;
    Decryptor &operator=(const Decryptor &) = delete;
    Decryptor(Decryptor &&) = default;
    Decryptor &operator=(Decryptor &&) = default;
    virtual ~Decryptor() = default;
    virtual void process_unit(std::shared_ptr<Unit> unit) = 0;
    [[nodiscard]] virtual Result get_best_result() const = 0;

protected:
    Decryptor() = default;
};

class PolyAlphabeticDecryptor : public Decryptor {
public:
    explicit PolyAlphabeticDecryptor(
        std::shared_ptr<const EncryptedMessage> message,
        const std::string &dict_path,
        const std::string &trigrams_path
    )
        : m_message(std::move(message)),
          m_best_result{"-1", std::numeric_limits<double>::max(), ""} {
        m_dict.load(dict_path);
        load_trigrams(trigrams_path);
    }

    void process_unit(std::shared_ptr<Unit> unit) override;

    [[nodiscard]] Result get_best_result() const override {
        return m_best_result;
    }

    void load_trigrams(const std::string& path);
    [[nodiscard]] double trigram_score(const std::string& text) const;

private:
    std::unordered_map<std::string, double> m_trigrams;
    double m_trigram_floor = 0.0;  // лог-вероятность для отсутствующих триграм
    bool m_has_trigrams = false;
    std::shared_ptr<const EncryptedMessage> m_message;
    Dictionary m_dict;
    Result m_best_result;
    std::array<std::array<int, 26>, 7> streams_freq{};
    std::array<int, 7> stream_size;
    void pre_calculation_score(const std::string &text);
    static double calculate_score(const std::string &text);
    double score_for_key(const std::string &key);
    static const double ENGLISH_FREQS[26];
};

}  // namespace server

#endif  // WORKER_HPP
