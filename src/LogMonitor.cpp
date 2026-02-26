#include "LogMonitor.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <vector>

LogMonitor::LogMonitor(const std::string& filename) : filename_(filename), fd_(-1), last_pos_(0) {}

LogMonitor::~LogMonitor() {
    if (fd_ != -1) close(fd_);
}

bool LogMonitor::open() {
    fd_ = ::open(filename_.c_str(), O_RDONLY);
    if (fd_ == -1) {
        perror("OPer: open log");
        return false;
    }
    // перейти в конец
    last_pos_ = lseek(fd_, 0, SEEK_END);
    return true;
}

bool LogMonitor::checkRotation() {
    struct stat st;
    if (fstat(fd_, &st) != 0) return false;
    // проверить, что inode не изменился
    struct stat st_name;
    if (stat(filename_.c_str(), &st_name) != 0) return false;
    if (st.st_ino != st_name.st_ino) {
        // файл был пересоздан (ротация)
        ::close(fd_);
        fd_ = ::open(filename_.c_str(), O_RDONLY);
        if (fd_ == -1) return false;
        last_pos_ = 0; // читаем с начала нового файла
    }
    return true;
}

std::vector<std::string> LogMonitor::getNewLines() {
    std::vector<std::string> lines;
    if (fd_ == -1) return lines;

    // проверить ротацию
    checkRotation();

    // определить текущий размер
    off_t cur_size = lseek(fd_, 0, SEEK_END);
    if (cur_size < last_pos_) {
        // файл мог быть урезан (редко), просто сбросим позицию
        last_pos_ = 0;
    }
    if (cur_size == last_pos_) {
        return lines; // ничего нового
    }

    // прочитать данные от last_pos_ до cur_size
    lseek(fd_, last_pos_, SEEK_SET);
    std::string buffer(cur_size - last_pos_, '\0');
    ssize_t n = read(fd_, &buffer[0], buffer.size());
    if (n > 0) {
        buffer.resize(n);
        // разбить на строки
        size_t start = 0;
        size_t end;
        while ((end = buffer.find('\n', start)) != std::string::npos) {
            lines.push_back(buffer.substr(start, end - start));
            start = end + 1;
        }
        if (start < buffer.size()) {
            lines.push_back(buffer.substr(start)); // последняя строка без \n
        }
    }
    last_pos_ = cur_size;
    return lines;
}