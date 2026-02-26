#include "ActionExecutor.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int ActionExecutor::runCommand(const std::string& cmd, int timeoutSec) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }
    int status;
    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = [](int) {};
    sigaction(SIGALRM, &sa, &old_sa);
    alarm(timeoutSec);
    pid_t w = waitpid(pid, &status, 0);
    int saved_errno = errno;
    alarm(0);
    sigaction(SIGALRM, &old_sa, nullptr);

    if (w == -1 && saved_errno == EINTR) {
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        return -2; // timeout
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

bool ActionExecutor::executeAction(const Action& act, int depth, bool& overall_failure) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << "Executing: " << act.command << " (timeout " << act.timeout << "s, ignore_failure=" << act.ignore_failure << ")" << std::endl;
    int ret = runCommand(act.command, act.timeout);
    bool failed = false;
    if (ret == -2) {
        std::cout << indent << "Timeout expired" << std::endl;
        if (act.has_on_timeout()) {
            std::cout << indent << "Executing on_timeout action" << std::endl;
            // on_timeout выполняется вне зависимости от ignore_failure? Да, это специальная ветка.
            // Здесь ignore_failure относится к этой ветке, но логика: если on_timeout сработал и у него свой ignore_failure.
            if (!executeAction(act.on_timeout.value(), depth + 1, overall_failure)) {
                failed = true;
            }
        } else {
            failed = true; // таймаут без обработчика считается ошибкой
        }
    } else if (ret != 0) {
        std::cout << indent << "Command failed with exit code " << ret << std::endl;
        failed = true;
    } else {
        std::cout << indent << "Command succeeded" << std::endl;
    }

    if (failed) {
        if (act.ignore_failure) {
            std::cout << indent << "Failure ignored, continuing." << std::endl;
            return true; // не прерываем цепочку, но отмечаем общий сбой, если нужно? Можно не отмечать.
        } else {
            overall_failure = true;
            return false; // прерываем цепочку
        }
    }
    return true;
}

void ActionExecutor::executeInternal(std::shared_ptr<Algorithm> algo) {
    std::cout << "Starting action sequence for pattern: " << algo->pattern_str << std::endl;
    bool overall_failure = false;
    for (size_t i = 0; i < algo->actions.size(); ++i) {
        std::cout << "Step " << i+1 << "/" << algo->actions.size() << std::endl;
        if (!executeAction(algo->actions[i], 1, overall_failure)) {
            std::cout << "Step failed with no ignore_failure, stopping sequence." << std::endl;
            break;
        }
    }
    if (overall_failure) {
        std::cout << "Action sequence completed with some failures (ignored)." << std::endl;
    } else {
        std::cout << "Action sequence completed successfully." << std::endl;
    }
}

std::future<void> ActionExecutor::executeAsync(std::shared_ptr<Algorithm> algo) {
    return std::async(std::launch::async, [algo]() { executeInternal(algo); });
}