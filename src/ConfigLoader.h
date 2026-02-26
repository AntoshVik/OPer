#pragma once

#include "Algorithm.h"
#include <string>
#include <vector>

class ConfigLoader {
public:
    // Загружает все .json файлы из указанной директории, возвращает список алгоритмов.
    static std::vector<Algorithm> loadFromDirectory(const std::string& dirPath);
};