#pragma once

#include <string>
#include <vector>

class LogMonitor {
public:
    explicit LogMonitor(const std::string& source);
    ~LogMonitor();

    bool open();
    std::vector<std::string> getNewLines();

private:
    std::string source_;
    int fd_;
    bool use_kmsg_;
    off_t last_pos_;          // для файлового режима
    std::string leftover_;     // для неполных строк в файловом режиме

    bool checkRotation();      // для файлового режима
};