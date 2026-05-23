#include "KasiskiAnalyzer.hpp"
#include <algorithm>
#include <map>
#include <numeric>
#include <vector>

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

int KasiskiAnalyzer::guessKeyLengthByIC(
    const std::string &ciphertext,
    int minLen,
    int maxLen
) {
    std::string cleanText;
    for (char c : ciphertext) {
        if (std::isalpha(c)) cleanText += std::toupper(c);
    }

    double bestIC = 0.0;
    int bestLength = minLen;

    for (int len = minLen; len <= maxLen; ++len) {
        double avgIC = 0.0;
        for (int i = 0; i < len; ++i) {
            std::string subsequence;
            for (size_t j = i; j < cleanText.size(); j += len) {
                subsequence += cleanText[j];
            }
            avgIC += calculateIndexOfCoincidence(subsequence);
        }
        avgIC /= len;
        if (avgIC > bestIC) {
            bestIC = avgIC;
            bestLength = len;
        }
    }

    return bestLength;
}

int KasiskiAnalyzer::guessKeyLength(
    const std::string &ciphertext,
    int minLen,
    int maxLen
) {
    maxLen = 10;
    std::string text;
    for (char c : ciphertext) {
        if (std::isalpha(c)) text += std::toupper(c);
    }

    // Считаем сколько раз каждый делитель встречается среди расстояний
    std::map<std::string, std::vector<int>> trigramPositions;
    for (size_t i = 0; i + 3 <= text.length(); ++i) {
        trigramPositions[text.substr(i, 3)].push_back(i);
    }

    std::map<int, int> divisorCount;
    for (const auto &[trigram, positions] : trigramPositions) {
        if (positions.size() < 2) continue;
        for (size_t j = 1; j < positions.size(); ++j) {
            int dist = positions[j] - positions[j - 1];
            for (int d = std::max(minLen, 2); d <= maxLen; ++d) {
                if (dist % d == 0) {
                    divisorCount[d]++;
                }
            }
        }
    }

    if (divisorCount.empty()) {
        return guessKeyLengthByIC(text, minLen, maxLen);
    }

    // Берём топ-3 кандидата по частоте делителей
    std::vector<std::pair<int, int>> candidates(
        divisorCount.begin(), divisorCount.end()
    );
    std::sort(candidates.begin(), candidates.end(), [](auto &a, auto &b) {
        return a.second > b.second;
    });

    int topN = std::min(3, (int)candidates.size());

    // Среди топ-3 выбираем лучший по IC
    int bestLen = candidates[0].first;
    double bestIC = 0.0;
    for (int i = 0; i < topN; ++i) {
        int len = candidates[i].first;
        double ic = 0.0;
        for (int pos = 0; pos < len; ++pos) {
            std::string sub;
            for (size_t j = pos; j < text.size(); j += len) {
                sub += text[j];
            }
            ic += calculateIndexOfCoincidence(sub);
        }
        ic /= len;
        if (ic > bestIC) {
            bestIC = ic;
            bestLen = len;
        }
    }

    return bestLen;
}
