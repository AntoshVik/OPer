#include "LogMonitor.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <string.h>
#include <iostream>
#include <errno.h>

LogMonitor::LogMonitor(const std::string& source) 
    : source_(source), fd_(-1), use_kmsg_(false), last_pos_(0), leftover_() 
{
    if (source == "/dev/kmsg" || source == "kmsg") {
        use_kmsg_ = true;
        source_ = "/dev/kmsg";
    }
}

LogMonitor::~LogMonitor() {
    if (fd_ != -1) close(fd_);
}

bool LogMonitor::open() {
    if (use_kmsg_) {
        fd_ = ::open(source_.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd_ == -1) {
            perror("OPer: open /dev/kmsg");
            return false;
        }
        // пропускаем старые сообщения
        char buf[4096];
        while (read(fd_, buf, sizeof(buf)) > 0) {}
        return true;
    } else {
        fd_ = ::open(source_.c_str(), O_RDONLY);
        if (fd_ == -1) {
            perror("OPer: open log file");
            return false;
        }
        last_pos_ = lseek(fd_, 0, SEEK_END);
        return true;
    }
}

bool LogMonitor::checkRotation() {
    struct stat st;
    if (fstat(fd_, &st) != 0) return false;
    struct stat st_name;
    if (stat(source_.c_str(), &st_name) != 0) return false;
    if (st.st_ino != st_name.st_ino) {
        // файл был пересоздан (ротация)
        ::close(fd_);
        fd_ = ::open(source_.c_str(), O_RDONLY);
        if (fd_ == -1) return false;
        last_pos_ = 0;
        leftover_.clear();
    }
    return true;
}

std::vector<std::string> LogMonitor::getNewLines() {
    std::vector<std::string> lines;
    if (fd_ == -1) return lines;

    if (use_kmsg_) {
        char buffer[8192];
        ssize_t n;
        while ((n = read(fd_, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            char *start = buffer;
            char *end;
            while ((end = strchr(start, '\n')) != nullptr) {
                *end = '\0';
                lines.push_back(start);
                start = end + 1;
            }
            if (start < buffer + n) {
                lines.push_back(start); // неполная строка (редко)
            }
        }
        if (n == -1 && errno != EAGAIN) {
            perror("OPer: read /dev/kmsg");
        }
    } else {
        checkRotation();
        off_t cur_size = lseek(fd_, 0, SEEK_END);
        if (cur_size < last_pos_) {
            last_pos_ = 0;
            leftover_.clear();
        }
        if (cur_size == last_pos_) return lines;

        lseek(fd_, last_pos_, SEEK_SET);
        std::string buffer(cur_size - last_pos_, '\0');
        ssize_t n = read(fd_, &buffer[0], buffer.size());
        if (n > 0) {
            buffer.resize(n);
            if (!leftover_.empty()) {
                buffer = leftover_ + buffer;
                leftover_.clear();
            }
            size_t start = 0, end;
            while ((end = buffer.find('\n', start)) != std::string::npos) {
                lines.push_back(buffer.substr(start, end - start));
                start = end + 1;
            }
            if (start < buffer.size()) {
                leftover_ = buffer.substr(start);
            }
        }
        last_pos_ = cur_size;
    }
    return lines;
}