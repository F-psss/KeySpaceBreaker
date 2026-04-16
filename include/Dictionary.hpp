#pragma once
#include <unordered_set>
#include <string>

class Dictionary {
public:
    void load(const std::string& path);
    double score(const std::string& text) const;

private:
    std::unordered_set<std::string> words;
};