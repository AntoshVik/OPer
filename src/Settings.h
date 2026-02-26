#pragma once

#include <string>
#include <chrono>

struct Settings {
    std::string log_file = "/var/log/kern.log";
    int reload_interval = 5;                // секунды
    std::string algorithms_dir = "/etc/oper/algorithms";
    int default_cooldown = 60;               // секунды, если не указано в алгоритме
    int max_parallel_tasks = 5;               // максимальное количество одновременно выполняемых цепочек
};

// Загружает настройки из JSON-файла. Если файл не найден или повреждён, возвращает настройки по умолчанию.
Settings loadSettings(const std::string& config_path = "/etc/oper/oper.conf");