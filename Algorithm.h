#pragma once
#include <string>
#include <vector>
#include <optional>
#include <chrono>

struct Action {
    std::string command;
    int timeout;                    // секунды
    std::optional<Action> on_timeout;

    bool has_on_timeout() const { return on_timeout.has_value(); }
};

struct Algorithm {
    std::string pattern;
    int cooldown;                    // секунды
    std::vector<Action> actions;
    std::chrono::steady_clock::time_point last_trigger;
};