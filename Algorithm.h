#pragma once
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <regex>

struct Action {
    std::string command;
    int timeout;                       // секунды
    bool ignore_failure;                // продолжать цепочку даже при ошибке
    std::optional<Action> on_timeout;

    bool has_on_timeout() const { return on_timeout.has_value(); }
};

struct Algorithm {
    std::regex pattern;                 // регулярное выражение
    int cooldown;                        // секунды
    std::vector<Action> actions;
    std::chrono::steady_clock::time_point last_trigger;
    // для перезагрузки нужно знать исходную строку паттерна? можно сохранить, но не обязательно
    std::string pattern_str;             // сохраним для логов
};