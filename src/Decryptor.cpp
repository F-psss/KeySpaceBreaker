#include "Decryptor.hpp"
#include <iostream>
#include <limits>

namespace server {

const double BruteforceDecryptor::ENGLISH_FREQS[26] = {
    0.08167, 0.01492, 0.02782, 0.04253, 0.12702, 0.02228, 0.02015,  // A-G
    0.06094, 0.06966, 0.00153, 0.00772, 0.04025, 0.02406, 0.06749,  // H-N
    0.07507, 0.01929, 0.00095, 0.05987, 0.06327, 0.09056, 0.02758,  // O-U
    0.00978, 0.02360, 0.00150, 0.01974, 0.00074                     // V-Z
};

void BruteforceDecryptor::process_unit(std::shared_ptr<Unit> unit) {
    if (!m_message) {
        std::cout << "GECNJ\n";
        return;
    }

    // std::cout << "Worker starting unit [" << unit->get_start() << ", "
    //           << unit->get_end() << "]" << '\n';
    auto index_to_key = [](int idx, int len) {
        std::string key(len, 'A');
        for (int i = len - 1; i >= 0; --i) {
            key[i] = 'A' + (idx % 26);
            idx /= 26;
        }
        return key;
    };
    // Функция для инкремента ключа (AAA -> AAB -> AAC ... -> AAZ -> ABA)
    auto increment_key = [](const std::string &key) {
        std::string result = key;
        for (int i = result.size() - 1; i >= 0; --i) {
            if (result[i] < 'Z') {
                result[i]++;
                return result;
            }
            result[i] = 'A';
        }
        return result;
    };
    m_noise = unit->get_noise();
    std::string current_key =
        index_to_key(unit->get_start(), unit->get_key_length());
    for (long long key = unit->get_start(); key <= unit->get_end(); ++key) {
        std::string candidate_text = m_message->decrypt(current_key);
        const double freq_score = calculate_score(candidate_text);
        std::cout << "=============\n";
        std::cout << current_key << ' ' << freq_score << ' ' << key  << std::endl;
        std::cout << candidate_text << '\n';
        std::cout << "=============\n";
        double score = freq_score;
        if (m_noise != 0.0) {
            const double dict_score = m_dict.score(candidate_text);
            // одновременно учитывается словарь и частотный анализ
            score = freq_score * (1 - m_noise * m_noise) -
                    dict_score * m_noise * 100;
        }
        if (score < m_best_result.score_) {
            m_best_result.score_ = score;
            m_best_result.key_ = key;
            m_best_result.text_ = candidate_text;
        }
        if (key < unit->get_end()) {
            current_key = increment_key(current_key);
        }
    }

    unit->mark_as_done();
}

double BruteforceDecryptor::calculate_score(const std::string &text) {
    long count[26] = {0};
    long total_letters = 0;

    for (const char c : text) {
        if (std::isalpha(c) != 0) {
            const int index = std::tolower(c) - 'a';
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
        const double expected = total_letters * ENGLISH_FREQS[i];
        const double difference = count[i] - expected;
        sum_score += (difference * difference) / expected;
    }

    return sum_score;
}

}  // namespace server
