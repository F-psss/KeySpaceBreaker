#pragma once

#include <string>
#include <vector>
class KasiskiAnalyzer {
public:
    // возвращает предполагаемую длину ключа
    static int guessKeyLength(const std::string& ciphertext, int minLen, int maxLength = 20);
    static int guessKeyLengthByIC(const std::string& ciphertext, int minLen, int maxLength = 20);


    // Разбивает текст на N последовательностей (по модулю keyLength)
    static std::vector<std::string> splitByModulo(const std::string& text, int keyLength);
};