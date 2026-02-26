#include "ConfigLoader.h"
#include "LogMonitor.h"
#include "ActionExecutor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

static volatile sig_atomic_t running = 1;

void handleSignal(int) {
    running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    std::string configDir = "/etc/cifs-monitor/algorithms";
    if (argc > 1) configDir = argv[1];

    auto algorithms = ConfigLoader::loadFromDirectory(configDir);
    if (algorithms.empty()) {
        std::cerr << "No algorithms loaded. Exiting." << std::endl;
        return 1;
    }

    LogMonitor monitor("/var/log/kern.log");
    if (!monitor.open()) {
        std::cerr << "Cannot open /var/log/kern.log" << std::endl;
        return 1;
    }

    std::cout << "Monitoring started. Press Ctrl+C to stop." << std::endl;

    while (running) {
        auto lines = monitor.getNewLines();
        for (const auto& line : lines) {
            // Поиск совпадений
            for (auto& algo : algorithms) {
                if (line.find(algo.pattern) != std::string::npos) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        now - algo.last_trigger).count();
                    if (elapsed >= algo.cooldown) {
                        std::cout << "Match found: " << line << std::endl;
                        ActionExecutor::execute(algo);
                        algo.last_trigger = now;
                    } else {
                        std::cout << "Match ignored (cooldown active for " 
                                  << algo.cooldown - elapsed << "s)" << std::endl;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Shutting down." << std::endl;
    return 0;
}