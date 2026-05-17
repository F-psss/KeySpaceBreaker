#include "KasiskiAnalyzer.hpp"
#include <algorithm>
#include <map>
#include <numeric>
#include <vector>
static double calculateIndexOfCoincidence(const std::string &text);

int KasiskiAnalyzer::guessKeyLengthByIC(
    const std::string &ciphertext,
    int minLen,
    int maxLength
) {
    std::string cleanText;
    for (char c : ciphertext) {
        if (std::isalpha(c)) {
            cleanText += std::toupper(c);
        }
    }

    // Метод индекса совпадений
    double bestIC = 0.0;
    int bestLength = 3;

    for (int len = 2; len <= maxLength; ++len) {
        double avgIC = 0.0;

        for (int i = 0; i < len; ++i) {
            std::string subsequence;
            for (size_t j = i; j < cleanText.size(); j += len) {
                subsequence += cleanText[j];
            }
            avgIC += calculateIndexOfCoincidence(subsequence);
        }
        avgIC /= len;
        if (avgIC > bestIC && avgIC > 0.055) {
            bestIC = avgIC;
            bestLength = len;
        }
    }

    return bestLength;
}

std::vector<std::string>
KasiskiAnalyzer::splitByModulo(const std::string &text, int keyLength) {
    std::vector<std::string> result(keyLength);
    int letterIndex = 0;

    for (char c : text) {
        if (std::isalpha(c)) {
            result[letterIndex % keyLength] += c;
            letterIndex++;
        }
    }

    return result;
}
static double calculateIndexOfCoincidence(const std::string &text) {
    if (text.length() < 2) {
        return 0.0;
    }

    int freq[26] = {0};
    for (char c : text) {
        freq[std::toupper(c) - 'A']++;
    }

    double sum = 0.0;
    int n = text.length();
    for (int i = 0; i < 26; ++i) {
        sum += freq[i] * (freq[i] - 1);
    }

    return sum / (n * (n - 1));
}

int KasiskiAnalyzer::guessKeyLength(
    const std::string &ciphertext,
    int minLen,
    int maxLen
) {
    std::string text;
    for (char c : ciphertext) {
        if (std::isalpha(c)) {
            text += std::toupper(c);
        }
    }

    // Ищем повторяющиеся триграммы
    std::map<std::string, std::vector<int>> trigramPositions;
    for (size_t i = 0; i < text.length() - 3; ++i) {
        std::string trigram = text.substr(i, 3);
        trigramPositions[trigram].push_back(i);
    }

    // Собираем все расстояния между повторениями
    std::vector<int> distances;
    for (const auto &[trigram, positions] : trigramPositions) {
        if (positions.size() >= 2) {
            for (size_t j = 1; j < positions.size(); ++j) {
                int dist = positions[j] - positions[j - 1];
                distances.push_back(dist);
            }
        }
    }

    // Находим НОД всех расстояний
    int gcd_all = distances.empty() ? 0 : distances[0];
    for (int d : distances) {
        gcd_all = std::gcd(gcd_all, d);
    }

    // Длина ключа — это наименьший делитель НОД в заданном диапазоне
    if (gcd_all >= minLen) {
        for (int len = minLen; len <= maxLen; ++len) {
            if (gcd_all % len == 0) {
                return len;
            }
        }
    }

    // Если НОД не подходит, используем индекс совпадений как fallback
    return guessKeyLengthByIC(text, minLen, maxLen);
}
