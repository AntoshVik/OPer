#include "ConfigLoader.h"
#include "LogMonitor.h"
#include "ActionExecutor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <memory>
#include <filesystem>
#include <csignal>

static std::atomic<bool> running(true);
static std::shared_timed_mutex algorithms_mutex;
static std::vector<Algorithm> algorithms;  // теперь вектор объектов, не указателей
static std::string configDir;
static std::filesystem::file_time_type last_config_load;

void signal_handler(int) {
    running = false;
}

void reloadConfigIfChanged() {
    namespace fs = std::filesystem;
    auto configDirPath = fs::path(configDir);
    if (!fs::exists(configDirPath) || !fs::is_directory(configDirPath)) return;

    auto current_mtime = fs::last_write_time(configDirPath);
    bool changed = false;
    for (const auto& entry : fs::directory_iterator(configDirPath)) {
        if (entry.path().extension() != ".json") continue;
        auto file_mtime = fs::last_write_time(entry.path());
        if (file_mtime > current_mtime) {
            current_mtime = file_mtime;
        }
        if (file_mtime > last_config_load) {
            changed = true;
        }
    }
    if (!changed && current_mtime <= last_config_load) return;

    std::cout << "OPer: Config directory changed, reloading..." << std::endl;
    auto new_algorithms = ConfigLoader::loadFromDirectory(configDir);
    {
        std::unique_lock<std::shared_timed_mutex> lock(algorithms_mutex);
        algorithms = std::move(new_algorithms);  // теперь типы совпадают
        last_config_load = fs::file_time_type::clock::now();
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    configDir = "/etc/oper/algorithms";
    if (argc > 1) configDir = argv[1];

    algorithms = ConfigLoader::loadFromDirectory(configDir);
    if (algorithms.empty()) {
        std::cerr << "OPer: No algorithms loaded. Exiting." << std::endl;
        return 1;
    }
    namespace fs = std::filesystem;
    if (fs::exists(configDir)) {
        last_config_load = fs::last_write_time(configDir);
    }

    LogMonitor monitor("/var/log/kern.log");
    if (!monitor.open()) {
        std::cerr << "OPer: Cannot open /var/log/kern.log" << std::endl;
        return 1;
    }

    std::cout << "OPer started. Monitoring " << configDir << " for algorithms. Press Ctrl+C to stop." << std::endl;

    auto last_reload_check = std::chrono::steady_clock::now();

    while (running) {
        auto lines = monitor.getNewLines();

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_reload_check).count() >= 5) {
            reloadConfigIfChanged();
            last_reload_check = now;
        }

        if (!lines.empty()) {
            std::shared_lock<std::shared_timed_mutex> lock(algorithms_mutex);
            for (const auto& line : lines) {
                for (const auto& algo : algorithms) {
                    if (std::regex_search(line, algo.pattern)) {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            now - algo.last_trigger).count();
                        if (elapsed >= algo.cooldown) {
                            std::cout << "OPer: Match found: " << line << std::endl;

                            // Создаём shared_ptr из копии алгоритма для асинхронного выполнения
                            auto algo_copy = std::make_shared<Algorithm>(algo);
                            // Обновляем время последнего срабатывания в оригинале
                            const_cast<Algorithm&>(algo).last_trigger = now;

                            ActionExecutor::executeAsync(algo_copy);
                        } else {
                            std::cout << "OPer: Match ignored (cooldown active for "
                                      << algo.cooldown - elapsed << "s)" << std::endl;
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "OPer shutting down." << std::endl;
    return 0;
}