#include "ConfigLoader.h"
#include "LogMonitor.h"
#include "ActionExecutor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <filesystem>
#include <map>
#include <mutex>
#include <memory>

static volatile sig_atomic_t running = 1;
void handleSignal(int) { running = 0; }

// Глобальные данные, защищённые мьютексом
std::mutex g_algorithms_mutex;
std::vector<std::shared_ptr<Algorithm>> g_algorithms; // используем shared_ptr для передачи в потоки
std::map<std::string, std::filesystem::file_time_type> g_file_times;
std::string g_config_dir;

void loadConfig() {
    auto new_algs = ConfigLoader::loadFromDirectory(g_config_dir);
    std::vector<std::shared_ptr<Algorithm>> new_shared;
    new_shared.reserve(new_algs.size());
    for (auto& alg : new_algs) {
        new_shared.push_back(std::make_shared<Algorithm>(std::move(alg)));
    }
    {
        std::lock_guard<std::mutex> lock(g_algorithms_mutex);
        g_algorithms.swap(new_shared);
    }
    std::cout << "Configuration reloaded. Loaded " << new_shared.size() << " algorithms." << std::endl;
}

void checkConfigUpdates() {
    namespace fs = std::filesystem;
    bool changed = false;
    // Собираем текущие времена файлов .json
    std::map<std::string, fs::file_time_type> current_times;
    for (const auto& entry : fs::directory_iterator(g_config_dir)) {
        if (entry.path().extension() != ".json") continue;
        auto path = entry.path().string();
        auto ftime = fs::last_write_time(entry.path());
        current_times[path] = ftime;
        auto it = g_file_times.find(path);
        if (it == g_file_times.end() || it->second != ftime) {
            changed = true;
        }
    }
    // Проверим, не удалены ли файлы
    if (g_file_times.size() != current_times.size()) {
        changed = true;
    }
    if (changed) {
        loadConfig();
        g_file_times = std::move(current_times);
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    g_config_dir = "/etc/cifs-monitor/algorithms";
    if (argc > 1) g_config_dir = argv[1];

    // Начальная загрузка
    loadConfig();
    if (g_algorithms.empty()) {
        std::cerr << "No algorithms loaded. Exiting." << std::endl;
        return 1;
    }

    LogMonitor monitor("/var/log/kern.log");
    if (!monitor.open()) {
        std::cerr << "Cannot open /var/log/kern.log" << std::endl;
        return 1;
    }

    std::cout << "Monitoring started. Press Ctrl+C to stop." << std::endl;

    auto last_config_check = std::chrono::steady_clock::now();
    const auto config_check_interval = std::chrono::seconds(5);

    while (running) {
        auto lines = monitor.getNewLines();
        if (!lines.empty()) {
            // Копируем список алгоритмов под мьютексом, чтобы не блокировать на долго
            std::vector<std::shared_ptr<Algorithm>> algs_copy;
            {
                std::lock_guard<std::mutex> lock(g_algorithms_mutex);
                algs_copy = g_algorithms; // копируем shared_ptr, это дёшево
            }
            for (const auto& line : lines) {
                for (auto& algo : algs_copy) {
                    if (std::regex_search(line, algo->pattern)) {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            now - algo->last_trigger).count();
                        if (elapsed >= algo->cooldown) {
                            std::cout << "Match found: " << line << std::endl;
                            // Обновляем время в исходном алгоритме (под мьютексом)
                            {
                                std::lock_guard<std::mutex> lock(g_algorithms_mutex);
                                // нужно найти этот алгоритм в текущем списке по указателю? Но мы могли скопировать shared_ptr, он тот же объект, поэтому можно обновить напрямую.
                                // Однако алгоритм может быть удалён при перезагрузке, но пока мы держим shared_ptr, объект жив.
                                algo->last_trigger = now;
                            }
                            // Запускаем асинхронно
                            ActionExecutor::executeAsync(algo); // алгоритм будет жить благодаря shared_ptr
                        } else {
                            std::cout << "Match ignored (cooldown active for " 
                                      << algo->cooldown - elapsed << "s)" << std::endl;
                        }
                    }
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_config_check >= config_check_interval) {
            checkConfigUpdates();
            last_config_check = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Shutting down." << std::endl;
    return 0;
}