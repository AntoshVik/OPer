#pragma once
#include "Algorithm.h"
#include <string>

class ActionExecutor {
public:
    // Выполнить всю цепочку действий алгоритма синхронно.
    // Возвращает true, если все действия выполнены (даже с ошибками, но без фатальных сбоев).
    static bool execute(const Algorithm& algo);
private:
    // Рекурсивно выполнить одно действие с учётом таймаута и on_timeout
    static bool executeAction(const Action& act, int depth = 0);
    // Запустить команду с таймаутом, вернуть код завершения или специальное значение при таймауте
    static int runCommand(const std::string& cmd, int timeoutSec);
};