#include "ActionExecutor.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <iostream>
#include <chrono>

int ActionExecutor::runCommand(const std::string& cmd, int timeoutSec) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
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
        return -2; // специальное значение: таймаут
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1; // другой сбой
}

bool ActionExecutor::executeAction(const Action& act, int depth) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << "Executing: " << act.command << " (timeout " << act.timeout << "s)" << std::endl;
    int ret = runCommand(act.command, act.timeout);
    if (ret == -2) {
        std::cout << indent << "Timeout expired" << std::endl;
        if (act.has_on_timeout()) {
            std::cout << indent << "Executing on_timeout action" << std::endl;
            return executeAction(act.on_timeout.value(), depth + 1);
        }
        return false; // можно считать, что действие не выполнено успешно
    } else if (ret != 0) {
        std::cout << indent << "Command failed with exit code " << ret << std::endl;
        // можно продолжить или прервать – по условию продолжаем
    } else {
        std::cout << indent << "Command succeeded" << std::endl;
    }
    return true;
}

bool ActionExecutor::execute(const Algorithm& algo) {
    std::cout << "Starting action sequence for pattern: " << algo.pattern << std::endl;
    for (size_t i = 0; i < algo.actions.size(); ++i) {
        std::cout << "Step " << i+1 << "/" << algo.actions.size() << std::endl;
        if (!executeAction(algo.actions[i], 1)) {
            std::cout << "Step failed, stopping sequence." << std::endl;
            return false;
        }
    }
    std::cout << "Action sequence completed." << std::endl;
    return true;
}