#pragma once
#include "Algorithm.h"
#include <memory>
#include <future>

class ActionExecutor {
public:
    // Запустить выполнение алгоритма асинхронно. Возвращает future, по которому можно узнать о завершении.
    static std::future<void> executeAsync(std::shared_ptr<Algorithm> algo);
private:
    static void executeInternal(std::shared_ptr<Algorithm> algo);
    static bool executeAction(const Action& act, int depth, bool& overall_failure);
    static int runCommand(const std::string& cmd, int timeoutSec);
};