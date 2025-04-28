#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void configureLogFilePath(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(path, std::ios::app);
        if (!logFile_) {
            std::cerr << "[ERROR] Could not open log file for writing: " << path << std::endl;
        }
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!logFile_.is_open()) {
            std::cerr << "[ERROR] Log file is not configured or could not be opened." << std::endl;
            return;
        }
        std::string timestamp = getTimestamp();
        logFile_ << "[" << timestamp << "] " << message << std::endl;
        std::cout << "[" << timestamp << "] " << message << std::endl;
    }

private:
    Logger() {}
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    std::ofstream logFile_;
    std::mutex mutex_;
};