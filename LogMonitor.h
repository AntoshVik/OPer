#pragma once
#include <string>
#include <vector>

class LogMonitor {
public:
    explicit LogMonitor(const std::string& filename);
    bool open();                        // открыть файл, перейти в конец
    std::vector<std::string> getNewLines(); // прочитать новые строки (с обработкой ротации)
private:
    std::string filename_;
    int fd_;                             // файловый дескриптор
    off_t last_pos_;                      // последняя прочитанная позиция
    bool checkRotation();                 // проверка, не был ли файл пересоздан
};