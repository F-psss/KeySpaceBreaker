#include "Decryptor.hpp"
#include <iostream>
#include <limits>

namespace server {

const double CaesarDecryptor::ENGLISH_FREQS[26] = {
    0.08167, 0.01492, 0.02782, 0.04253, 0.12702, 0.02228, 0.02015,  // A-G
    0.06094, 0.06966, 0.00153, 0.00772, 0.04025, 0.02406, 0.06749,  // H-N
    0.07507, 0.01929, 0.00095, 0.05987, 0.06327, 0.09056, 0.02758,  // O-U
    0.00978, 0.02360, 0.00150, 0.01974, 0.00074                     // V-Z
};

void CaesarDecryptor::process_unit(std::shared_ptr<Unit> unit) {
    if (!m_message) {
        std::cout << "GECNJ\n";
        return;
    }

    std::cout << "Worker starting unit [" << unit->get_start() << ", "
              << unit->get_end() << "]" << std::endl;

    for (int key = unit->get_start(); key <= unit->get_end(); ++key) {
        std::string candidate_text = m_message->decrypt(key);

        double score = calculate_score(candidate_text);
        std::cout << "Score = " << score << "\n";

        if (score < m_best_result.score_) {
            m_best_result.score_ = score;
            m_best_result.key_ = key;
            m_best_result.text_ = candidate_text;
        }
    }

    unit->mark_as_done();
}

double CaesarDecryptor::calculate_score(const std::string &text) {
    long count[26] = {0};
    long total_letters = 0;

    for (char c : text) {
        if (std::isalpha(c) != 0) {
            int index = std::tolower(c) - 'a';
            if (index >= 0 && index < 26) {
                count[index]++;
                total_letters++;
            }
        }
    }

    if (total_letters == 0) {
        return std::numeric_limits<double>::max();
    }

    double sum_score = 0.0;
    for (int i = 0; i < 26; ++i) {
        double expected = total_letters * ENGLISH_FREQS[i];
        double difference = count[i] - expected;
        sum_score += (difference * difference) / expected;
    }

    return sum_score;
}

}  // namespace server
