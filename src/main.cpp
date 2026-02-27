#include "ConfigLoader.h"
#include "LogMonitor.h"
#include "ActionExecutor.h"
#include "Settings.h"
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
static std::vector<Algorithm> algorithms;
static Settings settings;                  // глобальные настройки
static std::filesystem::file_time_type last_config_load;

void signal_handler(int) {
    running = false;
}

void reloadConfigIfChanged() {
    namespace fs = std::filesystem;
    auto configDirPath = fs::path(settings.algorithms_dir);
    if (!fs::exists(configDirPath) || !fs::is_directory(configDirPath)) return;

    // Определяем самое свежее время модификации среди JSON-файлов
    fs::file_time_type latest_mtime = fs::file_time_type::min();
    bool changed = false;

    for (const auto& entry : fs::directory_iterator(configDirPath)) {
        if (entry.path().extension() != ".json") continue;
        auto file_mtime = fs::last_write_time(entry.path());
        if (file_mtime > latest_mtime) {
            latest_mtime = file_mtime;
        }
        if (file_mtime > last_config_load) {
            changed = true;
        }
    }

    if (!changed) return;

    std::cout << "OPer: Config directory changed, reloading..." << std::endl;
    auto new_algorithms = ConfigLoader::loadFromDirectory(settings.algorithms_dir, settings.default_cooldown);
    {
        std::unique_lock<std::shared_timed_mutex> lock(algorithms_mutex);
        algorithms = std::move(new_algorithms);
        last_config_load = latest_mtime; // обновляем на самое свежее время
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Загрузка настроек (можно передать путь как аргумент командной строки)
    std::string config_path = "/etc/oper/oper.conf";
    if (argc > 1) config_path = argv[1];
    settings = loadSettings(config_path);

    algorithms = ConfigLoader::loadFromDirectory(settings.algorithms_dir, settings.default_cooldown);
    if (algorithms.empty()) {
        std::cerr << "OPer: No algorithms loaded. Exiting." << std::endl;
        return 1;
    }
    namespace fs = std::filesystem;
    if (fs::exists(settings.algorithms_dir) && fs::is_directory(settings.algorithms_dir)) {
        last_config_load = fs::file_time_type::min();
        for (const auto& entry : fs::directory_iterator(settings.algorithms_dir)) {
            if (entry.path().extension() == ".json") {
                auto ftime = fs::last_write_time(entry.path());
                if (ftime > last_config_load)
                    last_config_load = ftime;
            }
        }
    } else {
        last_config_load = fs::file_time_type::clock::now();
    }

    LogMonitor monitor(settings.log_file);
    if (!monitor.open()) {
        std::cerr << "OPer: Cannot open log file: " << settings.log_file << std::endl;
        return 1;
    }

    std::cout << "OPer started. Monitoring " << settings.log_file
              << " for algorithms in " << settings.algorithms_dir
              << ". Press Ctrl+C to stop." << std::endl;

    auto last_reload_check = std::chrono::steady_clock::now();

    while (running) {
        auto lines = monitor.getNewLines();

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_reload_check).count() >= settings.reload_interval) {
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
                        if (elapsed >= algo.cooldown || algo.last_trigger == std::chrono::steady_clock::time_point::min() ) {
                            std::cout << "OPer: Match found: " << line << std::endl;

                            auto algo_copy = std::make_shared<Algorithm>(algo);
                            const_cast<Algorithm&>(algo).last_trigger = now;

                            // TODO: ограничение параллельных задач (max_parallel_tasks)
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