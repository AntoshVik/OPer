#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <regex>
#include <memory>

struct Action {
    std::string command;
    int timeout;                    // секунды
    bool ignore_failure;             // продолжать цепочку даже при ошибке команды?
    std::unique_ptr<Action> on_timeout;

    Action() : timeout(0), ignore_failure(false) {}
    
    // Конструктор копирования (глубокое копирование)
    Action(const Action& other)
        : command(other.command),
          timeout(other.timeout),
          ignore_failure(other.ignore_failure),
          on_timeout(other.on_timeout ? std::make_unique<Action>(*other.on_timeout) : nullptr)
    {}
    
    // Конструктор перемещения (по умолчанию)
    Action(Action&& other) = default;
    
    // Оператор присваивания копированием
    Action& operator=(const Action& other) {
        if (this != &other) {
            command = other.command;
            timeout = other.timeout;
            ignore_failure = other.ignore_failure;
            on_timeout = other.on_timeout ? std::make_unique<Action>(*other.on_timeout) : nullptr;
        }
        return *this;
    }
    
    // Оператор присваивания перемещением
    Action& operator=(Action&& other) = default;
    
    ~Action() = default;

    bool has_on_timeout() const { return on_timeout != nullptr; }
};

struct Algorithm {
    std::regex pattern;
    int cooldown;                    // секунды между срабатываниями
    std::vector<Action> actions;
    std::chrono::steady_clock::time_point last_trigger;
    mutable std::shared_ptr<int> alive_flag; // для отслеживания времени жизни копий

    Algorithm() : cooldown(0), alive_flag(std::make_shared<int>(0)) {}
    
    // Конструктор копирования
    Algorithm(const Algorithm& other)
        : pattern(other.pattern),
          cooldown(other.cooldown),
          actions(other.actions),
          last_trigger(other.last_trigger),
          alive_flag(std::make_shared<int>(0)) // новый флаг, не копируем
    {}
    
    // Конструктор перемещения
    Algorithm(Algorithm&& other) = default;
    
    // Оператор присваивания копированием
    Algorithm& operator=(const Algorithm& other) {
        if (this != &other) {
            pattern = other.pattern;
            cooldown = other.cooldown;
            actions = other.actions;
            last_trigger = other.last_trigger;
            alive_flag = std::make_shared<int>(0); // новый флаг
        }
        return *this;
    }
    
    // Оператор присваивания перемещением
    Algorithm& operator=(Algorithm&& other) = default;
    
    ~Algorithm() = default;
};