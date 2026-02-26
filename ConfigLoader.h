#pragma once
#include "Algorithm.h"
#include <vector>
#include <string>

class ConfigLoader {
public:
    static std::vector<Algorithm> loadFromDirectory(const std::string& dirPath);
};