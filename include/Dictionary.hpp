#pragma once
#include <string>
#include <unordered_set>

class Dictionary {
public:
    void load(const std::string &path);
    double score(const std::string &text) const;

private:
    std::unordered_set<std::string> words;
};