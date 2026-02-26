#pragma once

#include "Algorithm.h"
#include <memory>
#include <future>

class ActionExecutor {
public:
    // Асинхронно выполняет цепочку действий алгоритма.
    // Возвращает future, по которому можно отследить завершение (но обычно не требуется).
    static std::future<void> executeAsync(std::shared_ptr<Algorithm> algo);
};