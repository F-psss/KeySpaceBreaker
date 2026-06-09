#include "Decryptor.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>
#include <cmath>

namespace server {
const double PolyAlphabeticDecryptor::ENGLISH_FREQS[26] = {
    0.08167, 0.01492, 0.02782, 0.04253, 0.12702, 0.02228, 0.02015,  // A-G
    0.06094, 0.06966, 0.00153, 0.00772, 0.04025, 0.02406, 0.06749,  // H-N
    0.07507, 0.01929, 0.00095, 0.05987, 0.06327, 0.09056, 0.02758,  // O-U
    0.00978, 0.02360, 0.00150, 0.01974, 0.00074                     // V-Z
};

double PolyAlphabeticDecryptor::score_for_key(const std::string &key) {
    double total = 0;
    for (int i = 0; i < key.size(); ++i) {
        int shift = key[i] - 'A';
        int n = stream_size[i];
        for (int c = 0; c < 26; ++c) {
            // после сдвига буква (c - shift + 26) % 26 становится c
            double observed = streams_freq[i][(c + shift) % 26];
            double expected = n * ENGLISH_FREQS[c];
            double diff = observed - expected;
            total += diff * diff / expected;
        }
    }
    return total;
}

void PolyAlphabeticDecryptor::process_unit(std::shared_ptr<Unit> unit) {
    if (!m_message) {
        std::cout << "GECNJ\n";
        return;
    }
    if (unit->get_mode() == decrypt::VigenereMode::FAST) {
        const int key_len = unit->get_key_length();
        // обнуляем
        for (int i = 0; i < key_len; ++i) {
            stream_size[i] = 0;
            streams_freq[i].fill(0);
        }
        // заполняем частоты по потокам
        std::string text = m_message->get_text();
        int letter_idx = 0;
        for (char c : text) {
            if (std::isalpha(c)) {
                int pos = letter_idx % key_len;
                streams_freq[pos][std::tolower(c) - 'a']++;
                stream_size[pos]++;
                letter_idx++;
            }
        }
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
    const double noise = unit->get_noise();
    std::string current_key =
        index_to_key(unit->get_start(), unit->get_key_length());
    for (long long key = unit->get_start(); key <= unit->get_end(); ++key) {
        double score = 0;
        if (unit->get_mode() == decrypt::VigenereMode::FAST) {
            score = score_for_key(current_key);
        } else {
            std::string candidate_text = m_message->decrypt(current_key);
            if (m_has_trigrams) {
                score = -trigram_score(candidate_text);
            } else {
                // критерий пирсона как fallback
                const double freq_score = calculate_score(candidate_text);
                score = freq_score;
                if (noise != 0.0) {
                    const double dict_score = m_dict.score(candidate_text);
                    score = freq_score * (1 - noise * noise) - dict_score * noise * 100;
                }
            }
        }
        if (score < m_best_result.score_) {
            m_best_result.score_ = score;
            m_best_result.key_ = current_key;
        }
        if (key < unit->get_end()) {
            current_key = increment_key(current_key);
        }
    }
    m_best_result.text_ = m_message->decrypt(m_best_result.key_);
    unit->mark_as_done();
}

void PolyAlphabeticDecryptor::load_trigrams(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open trigram file: " << path << std::endl;
        return;
    }

    std::string tri;
    double log_prob;
    double min_log = 0.0;
    while (file >> tri >> log_prob) {
        m_trigrams[tri] = log_prob;
        if (log_prob < min_log) {
            min_log = log_prob;
        }
    }

    m_trigram_floor = min_log - 1.0;
    m_has_trigrams = !m_trigrams.empty();
}

double PolyAlphabeticDecryptor::trigram_score(const std::string& text) const {
    std::string clean;
    clean.reserve(text.size());
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            clean += std::toupper(c);
        }
    }

    if (clean.size() < 3) {
        return m_trigram_floor * 10;
    }

    double score = 0.0;
    for (std::size_t i = 0; i + 3 <= clean.size(); ++i) {
        std::string tri = clean.substr(i, 3);
        auto it = m_trigrams.find(tri);
        score += (it != m_trigrams.end()) ? it->second : m_trigram_floor;
    }
    return score;
}

double PolyAlphabeticDecryptor::calculate_score(const std::string &text) {
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
