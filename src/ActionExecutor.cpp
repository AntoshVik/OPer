#include "ActionExecutor.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <iostream>
#include <future>
#include <chrono>
#include <cstring>

// Вспомогательная функция: выполнить команду с таймаутом.
// Возвращает:
//   0-255: код завершения команды
//   -1: ошибка fork/exec
//   -2: таймаут
static int runCommand(const std::string& cmd, int timeoutSec) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("OPer: fork");
        return -1;
    }
    if (pid == 0) {
        // дочерний процесс
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127); // если exec не удался
    }
    // родитель
    int status;
    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = [](int) {}; // пустой обработчик, только чтобы прервать waitpid
    sigaction(SIGALRM, &sa, &old_sa);
    alarm(timeoutSec);
    pid_t w = waitpid(pid, &status, 0);
    int saved_errno = errno;
    alarm(0);
    sigaction(SIGALRM, &old_sa, nullptr);

    if (w == -1 && saved_errno == EINTR) {
        // таймаут
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0); // убрать зомби
        return -2;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1; // другой сбой
}

// Рекурсивное выполнение одного действия
static bool executeAction(const Action& act, int depth = 0) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << "OPer: Executing: " << act.command
              << " (timeout " << act.timeout << "s, ignore_failure=" << act.ignore_failure << ")" << std::endl;

    int ret = runCommand(act.command, act.timeout);
    bool success = (ret == 0);

    if (ret == -2) {
        std::cout << indent << "OPer: Timeout expired for command" << std::endl;
        if (act.has_on_timeout()) {
            std::cout << indent << "OPer: Executing on_timeout action" << std::endl;
            success = executeAction(act.on_timeout.value(), depth + 1);
        } else {
            success = false;
        }
    } else if (ret != 0) {
        std::cout << indent << "OPer: Command failed with exit code " << ret << std::endl;
        success = false;
    } else {
        std::cout << indent << "OPer: Command succeeded" << std::endl;
        success = true;
    }

    if (!success && act.ignore_failure) {
        std::cout << indent << "OPer: Ignoring failure and continuing chain" << std::endl;
        return true; // говорим, что шаг успешен для цепочки
    }
    return success;
}

// Функция, выполняющая полную цепочку действий (запускается в отдельном потоке)
static void executeChain(std::shared_ptr<Algorithm> algo) {
    std::cout << "OPer: Starting action sequence for pattern" << std::endl;
    for (size_t i = 0; i < algo->actions.size(); ++i) {
        std::cout << "OPer: Step " << i+1 << "/" << algo->actions.size() << std::endl;
        bool stepResult = executeAction(algo->actions[i], 1);
        if (!stepResult) {
            std::cout << "OPer: Step failed, stopping sequence." << std::endl;
            return;
        }
    }
    std::cout << "OPer: Action sequence completed." << std::endl;
}

std::future<void> ActionExecutor::executeAsync(std::shared_ptr<Algorithm> algo) {
    return std::async(std::launch::async, executeChain, algo);
}