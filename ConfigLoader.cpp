#include "ConfigLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

static Action parseAction(const json& j) {
    Action act;
    act.command = j.at("command").get<std::string>();
    act.timeout = j.value("timeout", 30);
    if (j.contains("on_timeout") && !j.at("on_timeout").is_null()) {
        act.on_timeout = parseAction(j.at("on_timeout"));
    }
    return act;
}

std::vector<Algorithm> ConfigLoader::loadFromDirectory(const std::string& dirPath) {
    std::vector<Algorithm> algorithms;
    namespace fs = std::filesystem;

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.path().extension() != ".json") continue;
        std::ifstream f(entry.path());
        if (!f.is_open()) {
            std::cerr << "Cannot open " << entry.path() << std::endl;
            continue;
        }
        try {
            json j;
            f >> j;
            Algorithm algo;
            algo.pattern = j.at("pattern").get<std::string>();
            algo.cooldown = j.value("cooldown", 0);
            algo.last_trigger = std::chrono::steady_clock::time_point::min();

            for (const auto& act_j : j.at("actions")) {
                algo.actions.push_back(parseAction(act_j));
            }
            algorithms.push_back(std::move(algo));
            std::cout << "Loaded algorithm: " << algo.pattern << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing " << entry.path() << ": " << e.what() << std::endl;
        }
    }
    return algorithms;
}