#include "Dictionary.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

void Dictionary::load(const std::string &path) {
    std::ifstream file(path);
    std::string word;

    while (file >> word) {
        std::transform(word.begin(), word.end(), word.begin(), ::toupper);
        words.insert(word);
    }
}

double Dictionary::score(const std::string &text) const {
    std::istringstream iss(text);
    std::string word;
    int total = 0;
    int found = 0;
    while (iss >> word) {
        std::transform(word.begin(), word.end(), word.begin(), ::toupper);
        if (words.contains(word)) {
            std::cout << word << ' ';
            found++;
        }
        total++;
    }
    std::cout << '\n';
    return static_cast<double>(found) / total;
}