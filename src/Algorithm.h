#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <regex>
#include <memory>

struct Action {
    std::string command;
    int timeout;                    // секунды
    bool ignore_failure;             // продолжать цепочку даже при ошибке команды?
    std::optional<Action> on_timeout;

    bool has_on_timeout() const { return on_timeout.has_value(); }
};

struct Algorithm {
    std::regex pattern;
    int cooldown;                    // секунды между срабатываниями
    std::vector<Action> actions;
    std::chrono::steady_clock::time_point last_trigger;
    mutable std::shared_ptr<int> alive_flag; // для отслеживания времени жизни копий в асинхронных задачах

    Algorithm() : cooldown(0), alive_flag(std::make_shared<int>(0)) {}
    Algorithm(const Algorithm&) = default;
    Algorithm(Algorithm&&) = default;
    Algorithm& operator=(const Algorithm&) = default;
    Algorithm& operator=(Algorithm&&) = default;
};