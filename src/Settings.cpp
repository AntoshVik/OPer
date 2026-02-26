#include "Settings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Settings loadSettings(const std::string& config_path) {
    Settings sets;
    std::ifstream f(config_path);
    if (!f.is_open()) {
        std::cerr << "OPer: Config file not found, using default settings." << std::endl;
        return sets;
    }
    try {
        json j;
        f >> j;
        if (j.contains("log_file") && j["log_file"].is_string())
            sets.log_file = j["log_file"].get<std::string>();
        if (j.contains("reload_interval") && j["reload_interval"].is_number_integer())
            sets.reload_interval = j["reload_interval"].get<int>();
        if (j.contains("algorithms_dir") && j["algorithms_dir"].is_string())
            sets.algorithms_dir = j["algorithms_dir"].get<std::string>();
        if (j.contains("default_cooldown") && j["default_cooldown"].is_number_integer())
            sets.default_cooldown = j["default_cooldown"].get<int>();
        if (j.contains("max_parallel_tasks") && j["max_parallel_tasks"].is_number_integer())
            sets.max_parallel_tasks = j["max_parallel_tasks"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "OPer: Error parsing config file: " << e.what() << ". Using defaults." << std::endl;
    }
    return sets;
}